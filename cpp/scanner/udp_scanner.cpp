#include <atomic>
#include <iostream>
#include <thread>
#include <string>
#include <memory>

#include <unistd.h>
#include <sys/wait.h>

#include <signal.h>
#include <cstring>
#include <cstdlib>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "../log/log_service.hpp"
#include "../system/system_utilities.hpp"
#include "udp_scanner_listener.hpp"
#include "udp_scanner_sender.hpp"
#include "tcp_scanner.hpp"
#include "message_queue_packet.hpp"
#include "monitor.hpp"

#define MESSAGE_QUEUE_SIZE 100000
#define DEFAULT_SYS_BUFFER_SIZE 17000000

typedef boost::interprocess::message_queue msg_q;

#define PARENT_CLEAN_UP() std::cout << "[General] Doing cleaning up\n"; \
message_pack stop_signal; \
stop_signal.query_type = -1; \
message_queue->send(&stop_signal, sizeof(stop_signal), 2); \
sender.stop_send(); \
std::cout << "[General] Stopping monitor\n"; \
monitor.stop_monitor(); \
std::cout << "[General] Stopping io services\n"; \
io_service_listener.stop(); \
io_service_sender.stop(); \
std::cout << "[General] Stopping threads\n"; 

int main(int argc, char** argv)
{
    /* ----------- Parse User Inputs ----------- */
    boost::program_options::options_description option_set("Allowed options");
    option_set.add_options()
        ("help", "Help message")
        ("file_path", boost::program_options::value<std::string>(), "input path of the file")
        ("send_rate", boost::program_options::value<uint32_t>(),    "packet send rate")
        ("perc_of_addr", boost::program_options::value<float>(), "percent of scanning addresses")
        ("sys_rmem", boost::program_options::value<uint32_t>(), "system level socket buffer size")
        ("start_line", boost::program_options::value<uint32_t>(), "start from which line")
    ;

    boost::program_options::variables_map variables_map_;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, option_set), 
        variables_map_
    );
    boost::program_options::notify(variables_map_);  
  
    if (variables_map_.count("file_path") <= 0)
    {
        return -1;
    }

    std::uint32_t sys_sock_buf_size = 
    variables_map_.count("sys_rmem") > 0          ? 
    variables_map_["sys_rmem"].as<std::uint32_t>(): 
    DEFAULT_SYS_BUFFER_SIZE;

    std::uint32_t send_rate = 
    variables_map_.count("send_rate") > 0          ? 
    variables_map_["send_rate"].as<std::uint32_t>() : 
    40000;

    std::uint32_t start_line = 
    variables_map_.count("start_line") > 0          ? 
    variables_map_["start_line"].as<std::uint32_t>() : 
    0;

    float perc_of_addr = 
    variables_map_.count("perc_of_addr") > 0   ? 
    variables_map_["perc_of_addr"].as<float>() : 
    1.0;

    std::string file_path = variables_map_["file_path"].as<std::string>();
    /* ----------- Parse User Inputs END ----------- */

    modify_sysctl_rmem(sys_sock_buf_size);

    init_log_service(INITIALIZE_ON_SCANNER);

    boost::interprocess::message_queue::remove("pipe_to_tcp");

    std::shared_ptr<msg_q> message_queue = 
    std::make_shared<msg_q>(
        boost::interprocess::create_only,
        "pipe_to_tcp",
        MESSAGE_QUEUE_SIZE,
        sizeof(message_pack)
    );

    pid_t process_id = fork();

    if (process_id == -1)
    {
        std::cout << "[Scanner General] Fork failed, exiting...\n";
        std::exit(EXIT_FAILURE);
    }

    if (process_id == 0)
    {
        REGISTER_INTERRUPTION(child)

        try
        {
            TCPScanner scanner;
            scanner.service_loop();
        }
        catch(const KeyboardInterruption& e)
        {
            std::cout << "[General] child caught the exception\n";
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        REGISTER_INTERRUPTION(parent)
            
        boost::thread_group thread_pool_;

        std::atomic<bool> sender_wait_flag{false};
        std::atomic<bool> ddos_hold_on_flag{false};

        boost::asio::io_service io_service_listener;
        boost::asio::io_service io_service_sender;

        UDPSender sender(
            file_path, 
            send_rate, 
            start_line,
            perc_of_addr,
            io_service_sender, 
            message_queue, 
            sender_wait_flag,
            ddos_hold_on_flag
        );

        UDPListener listener(io_service_listener, message_queue, ddos_hold_on_flag);

        BufferMonitor monitor(message_queue, listener.get_socket(), sender_wait_flag, ddos_hold_on_flag);

        try
        {
            unsigned int number_of_threads = std::thread::hardware_concurrency() > 0 ?
            std::thread::hardware_concurrency() : 4;

            std::cout << "[Scanner General] Number of threads (listener+sender): " <<  number_of_threads << "\n";

            thread_pool_.create_thread(
                [&monitor](){ monitor.start_monitor(); }
            );

            thread_pool_.create_thread(
                [&sender](){ sender.start_send(); }
            );

            for(unsigned int index = 0; index < number_of_threads - 2; index++)
            {
                thread_pool_.create_thread(
                    [&io_service_listener](){ io_service_listener.run(); }
                );
            }

            thread_pool_.join_all();
            std::cout << "[Scanner General] stopping without keyboard interrupt\n";
            std::cout << "[Scanner General] Emergency status: " << ddos_hold_on_flag << "\n";
            
            PARENT_CLEAN_UP()
            std::cout << "[General] Clean up finished\n";

        }
        catch(const KeyboardInterruption& e)
        {
            PARENT_CLEAN_UP()
            thread_pool_.join_all(); 
            std::cout << "[General] Clean up finished\n";
        }

        try
        {
            std::cout << "[General] Starting waiting\n";
            int status;
            waitpid(-1, &status, 0);
            if (WIFEXITED(status))
            {
                std::cout << "[Scanner General] Child Exit Status: " << WEXITSTATUS(status) << std::endl;
            }
            else if (WIFSIGNALED(status))
            {
                std::cout 
                << "[Scanner General] Child Does not exit normally (signaled); signaled with "
                << ::strsignal(WTERMSIG(status))
                << "\n";
            }
            else if (WIFSTOPPED(status))
            {
                std::cout << "[Scanner General] Child Does not exit normally (stopped)\n";
            }
            else if (WIFCONTINUED(status))
            {
                std::cout << "[Scanner General] Child Does not exit normally (continued)\n";
            }
            else
            {
                std::cout << "[Scanner General] Child Does not exit normally (unknown status)\n";
            }
            return EXIT_SUCCESS;
        }
        catch(const KeyboardInterruption& e)
        {
            return EXIT_SUCCESS;
        }
    }
}

