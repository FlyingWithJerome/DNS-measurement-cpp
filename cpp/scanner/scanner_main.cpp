#include <iostream>
#include <string>

#include <boost/asio.hpp>
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

    boost::asio::io_context io_context;
    UDPListener listener(io_context);
    UDPSender   sender(file_path, io_context);

    io_context.run();

    return 0;
}

