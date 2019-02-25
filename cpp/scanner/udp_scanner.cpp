#include <iostream>
#include <thread>
#include <string>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "udp_scanner_listener.hpp"
#include "udp_scanner_sender.hpp"

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

    boost::asio::io_service io_service_listener;
    boost::asio::io_service io_service_sender;

    unsigned int number_of_threads = std::thread::hardware_concurrency() > 0 ?
    std::thread::hardware_concurrency() : 16;

    std::cout << "[Scanner General] Number of threads (listener+sender): " <<  number_of_threads << "\n";

    boost::thread_group thread_pool_;
    UDPListener listener(io_service_sender);
    UDPSender   sender(file_path, io_service_listener);

    sender.start_send();

    thread_pool_.create_thread(
        [&io_service_sender](){io_service_sender.run();}
    );

    for(unsigned int index = 0; index < number_of_threads - 1; index++)
    {
        thread_pool_.create_thread(
            [&io_service_listener](){io_service_listener.run();}
        );
    }

    thread_pool_.join_all();

    return 0;
}

