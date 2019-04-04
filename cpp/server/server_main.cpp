#include <thread>
#include <vector>
#include <boost/thread.hpp>
// #define UDP_SERVER_FROM_EXTERNAL_
// #define TCP_SERVER_FROM_EXTERNAL_
#include "udp_server.hpp"
#include "tcp_server.hpp"
#include "../log/log_service.hpp"
#include "../system/system_utilities.hpp"

int main()
{
    try
    {
        
        // std::thread udp_thread( [&](){udp_service.run();} );
        // std::thread tcp_thread( [&](){tcp_service.run();} );

        // udp_thread.join();
        // tcp_thread.join();

        modify_sysctl_rmem(17000000);

        init_log_service(INITIALIZE_ON_SERVER);

        boost::asio::io_service service;

        UDPServer udp_server(service);
        TCPServer tcp_server(service);

        unsigned int number_of_threads = 
        std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 16;

        boost::thread_group thread_pool_;

        std::cout << "[Server Global] server runs in " << number_of_threads << " threads\n";

        for(unsigned int index = 0; index < number_of_threads - 1; index++)
        {
            thread_pool_.create_thread(
                [&service](){service.run();}
            );
        }
        
        thread_pool_.join_all();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
