#include "tcp_scanner.hpp"

constexpr char TCPScanner::tcp_normal_log_[];

TCPScanner::TCPScanner()
: work(
    new boost::asio::io_service::work(io_service_)
)
{
    std::cout << "[TCP Scanner] Max number thread launched: " << std::thread::hardware_concurrency() << std::endl;
    try
    {
        pipe_to_tcp_.reset(
            new boost::interprocess::message_queue(
                boost::interprocess::open_only,
                "pipe_to_tcp"
            )
        );

        for (int index = 0; index < std::thread::hardware_concurrency(); index++)
        {
            thread_pool_.create_thread(
                boost::bind(&boost::asio::io_service::run, &io_service_)
            );
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    init_new_log_file(tcp_normal_log_);
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

            io_service_.post(
                boost::bind(
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
    TCPClient client(ip_address.c_str());
    client.connect();

    uint32_t question_id = NameTrick::get_question_id(question);

    if (not client.is_connected)
    {
        TCP_SCANNER_NORMAL_LOG(tcp_normal_log_, ip_address, "0", "-1", "connect timeout")
        return;
    }
    
    std::vector<uint8_t> full_packet;
    std::vector<uint8_t> response_packet;
    boost::system::error_code error;

    CRAFT_FULL_QUERY_TCP(question, full_packet)

    client.send(full_packet);

    if (client.receive(response_packet) <= 0)
    {
        TCP_SCANNER_NORMAL_LOG(tcp_normal_log_, ip_address, "0", "-1", "recv timeout")
        return;
    }

    try
    {
        Tins::DNS readable_response(response_packet.data()+2, response_packet.size()-2);

        std::cout << "[TCP Scanner] packet name: " << readable_response.answers()[0].dname() << std::endl;

        inspect_response(readable_response);

        TCP_SCANNER_NORMAL_LOG(tcp_normal_log_, ip_address, question_id, std::to_string(readable_response.rcode()), "answer ok")

    }
    catch (...)
    {
        std::cout << "[TCP Scanner] " << ip_address << " had malformed packet" <<  std::endl;
    }
}

void TCPScanner::inspect_response(Tins::DNS& response)
{
    if (response.rcode() != 0)
    {
        // the rcode is not even 0!
        return;
    }

    if (response.answers_count() != 100)
    {
        // the number of answers is not 100, write down the actual answer count
        return;
    }
}

TCPScanner::~TCPScanner()
{
    io_service_.stop();
    thread_pool_.join_all();
}


TCPScanner::TCPClient::TCPClient(
    const char* remote_addr
)
: socket_fd(
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
)
, is_connected(
    false
)
{
    remote_address.sin_family = AF_INET;
    remote_address.sin_port   = htons((uint16_t)53);

    int status = inet_aton(remote_addr, &remote_address.sin_addr);

    if (status == 0)
        std::cout << "the address is invalid: " << remote_addr << std::endl;

    fcntl(socket_fd, F_SETFL, O_NONBLOCK);

    FD_ZERO(&socket_set);
    FD_SET(socket_fd, &socket_set);
}

int TCPScanner::TCPClient::connect()
{
    int status = ::connect(
        socket_fd,
        (struct sockaddr*)&remote_address,
        sizeof(struct sockaddr)
    );

    int retval = select(socket_fd+1, nullptr, &socket_set, nullptr, &connect_timeout);
    int state;
    socklen_t size = sizeof(int);

    getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &state, &size);

    is_connected = (state == 0 and retval > 0);

    return is_connected;
}

int TCPScanner::TCPClient::send(const std::vector<uint8_t>& packet)
{
    int status = ::send(
        socket_fd,
        packet.data(),
        packet.size(),
        0
    );

    if (status < 0)
        std::cout << "[Send function] " << strerror(errno) << std::endl;        

    return status;
}

int TCPScanner::TCPClient::receive(std::vector<uint8_t>& packet)
{
    uint8_t packet_buffer[3000];

    int retval = select(socket_fd+1, &socket_set, nullptr, nullptr, &read_timeout);
    
    if (retval > 0)
    {
        int recv_size = ::recv(
            socket_fd,
            packet_buffer,
            3000,
            0
        );
        if (recv_size > 0)
        {
            packet.reserve(recv_size);
            std::copy_n(std::begin(packet_buffer), recv_size, std::back_inserter(packet));

            return recv_size;
        }
        else if (recv_size < 0)
        {
            std::cout << strerror(errno) << std::endl;
            return -1;
        }
        return 0;
    }
    return -1;
}

TCPScanner::TCPClient::~TCPClient()
{
    close(socket_fd);
}

int main()
{
    TCPScanner scanner;
    scanner.service_loop();

    return 0;
    // boost::asio::ip::tcp::endpoint remote_server_(
    //     boost::asio::ip::address::from_string("8.8.4.4"),
    //     53
    // );
    // TCPScanner::TCPClient client("8.8.3.3");

    // client.connect();

    // if (not client.is_connected)
    // {
    //     std::cout << "client had not been connected" << std::endl;
    //     return 1;
    // }

    // std::string question = "nogizaka.yumi.ipl.eecs.case.edu";
    // std::vector<uint8_t> packet;

    // CRAFT_FULL_QUERY_TCP(question, packet)

    // std::vector<uint8_t> response;

    // int send_bytes;
    // int recv_bytes;
    // if ((send_bytes = client.send(packet)) < 0)
    // {
    //     std::cout << "send error" << std::endl;
    // }
    // else
    // {
    //     std::cout << "send bytes " << send_bytes << std::endl;
    // }
    

    // if ((recv_bytes = client.receive(response)) < 0)
    // {
    //     std::cout << "recv error" << std::endl;
    // }
    // else
    // {
    //     std::cout << "recv bytes " << recv_bytes << std::endl;
    // }

    // std::cout << "has a response of size " << response.size() << std::endl;

    // return 0;
}
