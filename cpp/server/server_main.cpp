#include <signal.h>

#include <thread>
#include <vector>
#include <boost/thread.hpp>

#include "udp_server.hpp"
#include "tcp_server.hpp"
#include "../log/log_service.hpp"
#include "../system/system_utilities.hpp"

int main()
{
    REGISTER_INTERRUPTION(parent)
    boost::asio::io_service service;
    boost::thread_group thread_pool_;

    try
    {
        modify_sysctl_rmem(17000000);

        init_log_service(INITIALIZE_ON_SERVER);

        UDPServer udp_server(service);
        TCPServer tcp_server(service);

        unsigned int number_of_threads = 
        std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 16;

        std::cout << "[Server Global] server runs in " << number_of_threads << " threads\n";

        for(unsigned int index = 0; index < number_of_threads - 1; index++)
        {
            thread_pool_.create_thread(
                [&service](){service.run();}
            );
        }
        
        thread_pool_.join_all();
    }
    catch (KeyboardInterruption& k)
    {
        service.stop();
        thread_pool_.join_all();
        
        std::cerr << "[Scanner General] Cleaning up finished (keyboard interrupt)\n";
        return EXIT_SUCCESS;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;

        service.stop();
        thread_pool_.join_all();

        std::cerr << "[Scanner General] Cleaning up finished (unknown error)\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
