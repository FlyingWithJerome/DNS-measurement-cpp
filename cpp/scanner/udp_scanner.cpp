#include <iostream>
#include <thread>
#include <string>
#include <memory>

#include <unistd.h>
#include <sys/wait.h>

#include <signal.h>
#include <cstdlib>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "udp_scanner_listener.hpp"
#include "udp_scanner_sender.hpp"
#include "tcp_scanner.hpp"
#include "message_queue_packet.hpp"

static pid_t process_id;

typedef boost::interprocess::message_queue msg_q;

void keyboard_interruption_handler_child(int signal)
{
    std::cout << "[Scanner General] Going to Exit...\n";
    std::exit(1);
}

void keyboard_interruption_handler_parent(int signal)
{
    std::cout << "[Scanner General] Going to Exit...\n";

    int status;
    std::cout << "[Scanner General] Waiting for child @ " << process_id << std::endl;
    waitpid(process_id, &status, 0);
    if (WIFEXITED(status))
    {
        std::cout << "[Scanner General] Child Exit Status: " << WEXITSTATUS(status) << std::endl;
    }
    else
    {
        std::cout << "[Scanner General] Child Does not exit normally\n";
    }
    std::exit(1);
}

#define REGISTER_INTERRUPTION(identity) struct sigaction interruption_handler; \
interruption_handler.sa_handler = keyboard_interruption_handler_##identity; \
interruption_handler.sa_flags   = 0; \
sigemptyset(&interruption_handler.sa_mask); \
sigaction(SIGINT, &interruption_handler, nullptr);

int launch_udp_scanners(
    std::string&    file_path,
    std::shared_ptr<msg_q>& mq
)
{
    boost::asio::io_service io_service_listener;
    boost::asio::io_service io_service_sender;

    unsigned int number_of_threads = std::thread::hardware_concurrency() > 0 ?
    std::thread::hardware_concurrency() : 4;

    std::cout << "[Scanner General] Number of threads (listener+sender): " <<  number_of_threads << "\n";

    boost::thread_group thread_pool_;
    UDPSender   sender(file_path, io_service_sender, mq);
    UDPListener listener(io_service_listener, mq);

    thread_pool_.create_thread(
        [&sender](){ sender.start_send(); }
    );

    for(unsigned int index = 0; index < number_of_threads - 1; index++)
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
        ("send_rate", boost::program_options::value<uint32_t>(), "packet send rate")
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

    std::uint8_t send_rate = 
    variables_map_.count("send_rate") > 0          ? 
    variables_map_["send_rate"].as<std::uint8_t>() : 
    0;

    std::string file_path = variables_map_["file_path"].as<std::string>();
    /* ----------- Parse User Inputs END ----------- */


    boost::interprocess::message_queue::remove("pipe_to_tcp");

    std::shared_ptr<msg_q> message_queue = 
    std::make_shared<msg_q>(
        boost::interprocess::open_or_create,
        "pipe_to_tcp",
        100000,
        sizeof(message_pack)
    );

    process_id = fork();

    if (process_id == 0)
    {
        REGISTER_INTERRUPTION(child)
        TCPScanner scanner;
        scanner.service_loop();
        exit(0);
    }

    else
    {
        REGISTER_INTERRUPTION(parent)
        launch_udp_scanners(file_path, message_queue);
        int status;
        waitpid(process_id, &status, 0);
        exit(0);
    }
}

