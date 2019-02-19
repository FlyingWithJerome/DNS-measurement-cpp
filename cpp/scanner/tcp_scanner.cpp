#include "tcp_scanner.hpp"

TCPScanner::TCPScanner()
{
    try
    {
        pipe_to_tcp_.reset(
            new boost::interprocess::message_queue(
                boost::interprocess::open_only,
                "pipe_to_tcp"
            )
        );
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

int TCPScanner::service_loop(boost::asio::io_service& io_service_)
{
    try
    {
        while (true)
        {
            message_pack  empty;
            unsigned long recv_size;
            unsigned int priority = 1;

            pipe_to_tcp_->receive(&empty, sizeof(message_pack), recv_size, priority);

            std::cout << "retrieve one from mq: " << empty.ip_address << std::endl;

            thread_nest_.push_back(
                std::thread(
                    &TCPScanner::perform_tcp_query,
                    this,
                    std::ref(io_service_),
                    empty.ip_address
                )
            );
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    } 
}

void TCPScanner::perform_tcp_query(boost::asio::io_service& io_service_, const char* ip_address)
{
    std::cout << "the ip address is " << ip_address << std::endl;
}

TCPScanner::~TCPScanner()
{
    for(auto& member : thread_nest_)
    {
        member.join();
    }
    thread_nest_.clear();

    boost::interprocess::message_queue::remove("pipe_to_tcp");
}

int main()
{
    boost::asio::io_service io_service_;

    TCPScanner scanner;
    scanner.service_loop(io_service_);

    return 0;
}
