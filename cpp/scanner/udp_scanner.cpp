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

#include "../log/log_service.hpp"
#include "udp_scanner_listener.hpp"
#include "udp_scanner_sender.hpp"
#include "tcp_scanner.hpp"
#include "message_queue_packet.hpp"
#include "monitor.hpp"

#define MESSAGE_QUEUE_SIZE 100000
#define DEFAULT_SYS_BUFFER_SIZE 17000000

typedef boost::interprocess::message_queue msg_q;

void keyboard_interruption_handler_child(int);
void keyboard_interruption_handler_parent(int);

int launch_udp_scanners(
    const std::string&,
    const std::string&, 
    const std::uint32_t&, 
    const float&, 
    std::shared_ptr<msg_q>&
);

int modify_sysctl_rmem(const uint32_t&);

int modify_sysctl_rmem(const uint32_t& rmem_size)
{
    std::ofstream rmem_config("/proc/sys/net/core/rmem_max");
    if (rmem_config.is_open())
    {
        rmem_config << std::to_string(rmem_size) << "\n";
        return 0;
    }
    return -1;
}

void keyboard_interruption_handler_child(int signal)
{
    std::cout << "[Scanner General] Going to Exit (Child)...\n";
    std::exit(EXIT_SUCCESS);
}

void keyboard_interruption_handler_parent(int signal)
{
    std::cout << "[Scanner General] Going to Exit (Parent)...\n";

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
    std::exit(EXIT_SUCCESS);
}

#define REGISTER_INTERRUPTION(identity) struct sigaction interruption_handler; \
interruption_handler.sa_handler = keyboard_interruption_handler_##identity; \
interruption_handler.sa_flags   = 0; \
sigemptyset(&interruption_handler.sa_mask); \
sigaction(SIGINT, &interruption_handler, nullptr);


int launch_udp_scanners(
    const std::string&     file_path,
    const std::string&     type_of_query,
    const std::uint32_t&   send_rate,
    const float& percent_of_scanning_addr,
    std::shared_ptr<msg_q>& mq
)
{
    boost::asio::io_service io_service_listener;
    boost::asio::io_service io_service_sender;

    unsigned int number_of_threads = std::thread::hardware_concurrency() > 0 ?
    std::thread::hardware_concurrency() : 4;

    std::cout << "[Scanner General] Number of threads (listener+sender): " <<  number_of_threads << "\n";

    std::atomic<bool> sender_wait_flag{false};

    boost::thread_group thread_pool_;

    UDPSender sender(
        file_path, 
        type_of_query,
        send_rate, 
        percent_of_scanning_addr,
        io_service_sender, 
        mq, 
        sender_wait_flag
    );
    UDPListener listener(io_service_listener, mq);

    BufferMonitor monitor(mq, listener.get_socket(), sender_wait_flag);

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
}

int main(int argc, char** argv)
{
    /* ----------- Parse User Inputs ----------- */
    boost::program_options::options_description option_set("Allowed options");
    option_set.add_options()
        ("help", "Help message")
        ("file_path", boost::program_options::value<std::string>(), "input path of the file")
        ("send_rate", boost::program_options::value<uint32_t>(),    "packet send rate")
        ("perc_of_addr", boost::program_options::value<float>(), "percent of scanning addresses")
        ("type_of_query", boost::program_options::value<std::string>(), "type of DNS record used for query")
        ("sys_rmem", boost::program_options::value<uint32_t>(), "system level socket buffer size")
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

    float perc_of_addr = 
    variables_map_.count("perc_of_addr") > 0   ? 
    variables_map_["perc_of_addr"].as<float>() : 
    1.0;

    std::string type_of_query = 
    variables_map_.count("type_of_query") > 0         ? 
    variables_map_["type_of_query"].as<std::string>() : 
    std::string("A");

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

        TCPScanner scanner(type_of_query);
        scanner.service_loop();
        exit(EXIT_SUCCESS);
    }
    else
    {
        REGISTER_INTERRUPTION(parent)

        launch_udp_scanners(file_path, type_of_query, send_rate, perc_of_addr, message_queue);

        int status;
        waitpid(process_id, &status, 0);
        exit(EXIT_SUCCESS);
    }
}

