#include <iostream>
#include <thread>
#include <string>
#include <memory>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "udp_scanner_listener.hpp"
#include "udp_scanner_sender.hpp"
#include "tcp_scanner.hpp"

typedef struct{
    char ip_address[17];
    char question[70];
    // char hex_form[12];
    // unsigned int ip_in_int;
}message_pack;

int main(int argc, char** argv)
{
    boost::program_options::options_description option_set("Allowed options");
    option_set.add_options()
        ("help", "Help message")
        ("file_path", boost::program_options::value<std::string>(), "input path of the file")
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

    std::string file_path = variables_map_["file_path"].as<std::string>();

    boost::interprocess::message_queue::remove("pipe_to_tcp");

    std::shared_ptr<boost::interprocess::message_queue> message_queue = 
    std::make_shared<boost::interprocess::message_queue>(
        boost::interprocess::open_or_create,
        "pipe_to_tcp",
        100000,
        sizeof(message_pack)
    );

    boost::asio::io_service io_service_listener;
    boost::asio::io_service io_service_sender;

    unsigned int number_of_threads = std::thread::hardware_concurrency() > 0 ?
    std::thread::hardware_concurrency() : 4;

    std::cout << "[Scanner General] Number of threads (listener+sender): " <<  number_of_threads << "\n";

    boost::thread_group thread_pool_;
    UDPSender   sender(file_path, io_service_sender, message_queue);
    UDPListener listener(io_service_listener, message_queue);
    // sender.start_send();

    thread_pool_.create_thread(
        [&sender](){ sender.start_send(); }
    );

    for(unsigned int index = 0; index < number_of_threads - 1; index++)
    {
        thread_pool_.create_thread(
            [&io_service_listener](){ io_service_listener.run(); }
        );
    }

    // sender.start_send();

    thread_pool_.join_all();

    return 0;
}

