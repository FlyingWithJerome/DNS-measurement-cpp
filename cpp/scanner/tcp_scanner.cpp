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

int TCPScanner::service_loop()
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
                    std::string(empty.ip_address),
                    std::string(empty.question)
                )
            );
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    } 
}

void TCPScanner::perform_tcp_query(
    std::string ip_address,
    std::string question
)
{
    // std::unique_lock<std::mutex> lock_(mutex_, std::defer_lock);
    // lock_.lock();
    // std::cout << "the ip address is " << ip_address << std::endl;
    // lock_.unlock();

    boost::asio::ip::tcp::endpoint remote_server_(
        boost::asio::ip::address::from_string(ip_address),
        53
    );

    TCPClient client(remote_server_);

    client.connect();

    if (client.is_connected)
    {
        std::cout << "client had been connected" << std::endl;
    }
    
    std::vector<uint8_t> full_packet;
    boost::system::error_code error;

    CRAFT_FULL_QUERY_TCP(question, full_packet)
}

// void TCPScanner::prepare_socket(boost::asio::ip::tcp::socket& sock)
// {
//     struct timeval timeout = {8, 0};

//     setsockopt(
//         socket_fd,
//         SOL_SOCKET,
//         SO_RCVTIMEO,
//         (struct timeval*)&timeout,
//         sizeof(struct timeval)
//     );
// }

TCPScanner::~TCPScanner()
{
    for(auto& member : thread_nest_)
    {
        member.join();
    }
    thread_nest_.clear();

    boost::interprocess::message_queue::remove("pipe_to_tcp");
}

TCPScanner::TCPClient::TCPClient(
    boost::asio::ip::tcp::endpoint& remote_addr
)
: socket_(
    io_service_
)
, remote_endpoint_(
    remote_addr
)
, deadline_(
    io_service_
)
, is_connected(
    false
)
{
    deadline_.expires_at(boost::posix_time::pos_infin);
}

int TCPScanner::TCPClient::connect()
{
    deadline_.expires_from_now(boost::posix_time::seconds(3));

    boost::system::error_code error = boost::asio::error::would_block;

    socket_.async_connect(
        remote_endpoint_,
        boost::lambda::var(error) = boost::lambda::_1
    );

    do io_service_.run_one(); while (error == boost::asio::error::would_block);

    if (error || !socket_.is_open())
        throw boost::system::system_error(
            error ? error : boost::asio::error::operation_aborted
        );

    is_connected = true;

}

void TCPScanner::TCPClient::teardown()
{
    socket_.cancel();
    socket_.close();
}

void TCPScanner::TCPClient::handle_wait(const boost::system::error_code& error)
{
    if (error)
    {
        std::cout << "time out" << std::endl;
        teardown();
    }
}

void TCPScanner::TCPClient::handle_connect(const boost::system::error_code& error)
{
    if (not error)
    {
        std::cout << "successfully connected" << std::endl;
        deadline_.expires_at(boost::posix_time::pos_infin);
        is_connected = true;
    }
    else
    {
        std::cout << "connect error message " << error.message() << std::endl;
    }
    
}

void TCPScanner::TCPClient::check_deadline()
{
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        boost::system::error_code ec;
        socket_.close(ec);

        deadline_.expires_at(boost::posix_time::pos_infin);
    }

    deadline_.async_wait(boost::bind(&TCPClient::check_deadline, this));
}

int main()
{
    // TCPScanner scanner;
    // scanner.service_loop();

    // return 0;
    boost::asio::ip::tcp::endpoint remote_server_(
        boost::asio::ip::address::from_string("8.8.3.3"),
        53
    );
    TCPScanner::TCPClient client(remote_server_);

    client.connect();

    if (client.is_connected)
    {
        std::cout << "client had been connected" << std::endl;
    }

    return 0;
}
