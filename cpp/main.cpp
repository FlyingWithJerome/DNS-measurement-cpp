#include <thread>

// #define UDP_SERVER_FROM_EXTERNAL_
// #define TCP_SERVER_FROM_EXTERNAL_
#include "server/udp_server.hpp"
#include "server/tcp_server.hpp"
#include "log/log_service.hpp"

int main()
{
    try
    {
        // boost::asio::io_service udp_service;
        // UDPServer udp_server(udp_service);

        // boost::asio::io_service tcp_service;
        // TCPServer tcp_server(tcp_service);
        
        // std::thread udp_thread( [&](){udp_service.run();} );
        // std::thread tcp_thread( [&](){tcp_service.run();} );

        // udp_thread.join();
        // tcp_thread.join();

        // init_log_service();

        boost::asio::io_service service;

        UDPServer udp_server(service);
        TCPServer tcp_server(service);

        service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
