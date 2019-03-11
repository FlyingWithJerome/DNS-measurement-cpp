#include "tcp_scanner.hpp"

constexpr char TCPScanner::tcp_normal_log_[];

TCPScanner::TCPScanner()
: work(
    new boost::asio::io_service::work(io_service_)
)
{
    try
    {
        pipe_to_tcp_.reset(
            new boost::interprocess::message_queue(
                boost::interprocess::open_only,
                "pipe_to_tcp"
            )
        );

        std::cout << "[TCP Scanner] pipe to tcp size: " << pipe_to_tcp_->get_max_msg() << "\n";

        for (int index = 0; index < std::thread::hardware_concurrency() - 1; index++)
        {
            thread_pool_.create_thread(
                [&](){ io_service_.run(); }
            );
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[TCP Scanner] " << e.what() << std::endl;
    }

    init_new_log_file(tcp_normal_log_);
}

int TCPScanner::service_loop() noexcept
{

    while (true)
    {
        message_pack  empty;
        unsigned long recv_size = 0;
        unsigned int priority = 1;

        try
        {
            bool recv_status = pipe_to_tcp_->try_receive(&empty, sizeof(message_pack), recv_size, priority);

            if(recv_status and recv_size)
            {
                io_service_.post(
                    boost::bind(
                        &TCPScanner::perform_tcp_query,
                        this,
                        std::string(empty.ip_address),
                        std::string(empty.question)
                    )
                );
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::microseconds(50));
            }
        }
        catch(...)
        {
        }
    } 
}

void TCPScanner::perform_tcp_query(
    const std::string& ip_address,
    const std::string& question
)
{
    TCPClient client(ip_address.c_str());

    if (client.has_bad_fd)
    {
        return;
    }

    client.connect();

    uint32_t question_id = NameUtilities::get_question_id(question);

    if (not client.is_connected)
    {
        TCP_SCANNER_NORMAL_LOG(tcp_normal_log_, ip_address.c_str(), 0, -1, "connect_timeout")
        return;
    }
    
    std::vector<uint8_t> full_packet;
    std::vector<uint8_t> response_packet;

    packet_configuration packet_config;
    packet_config.id     = 1338;
    packet_config.q_name = question;

    packet_factory_.make_packet(
        PacketTypes::TCP_QUERY,
        packet_config,
        full_packet
    );
    // CRAFT_FULL_QUERY_TCP(question, full_packet)

    client.send(full_packet);
    
    if (client.receive(response_packet) <= 0)
    {
        TCP_SCANNER_NORMAL_LOG(tcp_normal_log_, ip_address.c_str(), 0, -1, "recv_timeout")
        return;
    }

    try
    {
        Tins::DNS readable_response(response_packet.data()+2, response_packet.size()-2);

        std::cout << "[TCP Scanner] packet name: " << readable_response.answers()[0].dname() << "\n";

        std::string result;
        inspect_response(readable_response, result);

        TCP_SCANNER_NORMAL_LOG(
            tcp_normal_log_, 
            ip_address.c_str(), 
            question_id, 
            readable_response.rcode(), 
            result.c_str()
        )

    }
    catch (...)
    {
        std::cout << "[TCP Scanner] " << ip_address << " had malformed packet\n";
        // TCP_SCANNER_NORMAL_LOG(
        //     tcp_normal_log_, 
        //     ip_address.c_str(), 
        //     question_id, 
        //     readable_response.rcode(), 
        //     "malformed_tcp_response"
        // )
    }
}

void TCPScanner::inspect_response(const Tins::DNS& response, std::string& result_str)
{
    if (response.rcode() != 0)
    {
        // the rcode is not even 0!
        std::stringstream result_builder;
        result_builder 
        << "non_zero_rcode(rcode:"
        << response.rcode()
        << ")";

        result_str = result_builder.str();
        return;
    }

    if (response.answers_count() != 100)
    {
        // the number of answers is not 100, write down the actual answer count
        std::stringstream result_builder;
        result_builder 
        << "wrong_number_of_entries("
        << response.answers_count()
        << "/100)";

        result_str = result_builder.str();
        return;
    }

    result_str = "answer_ok";
}

TCPScanner::~TCPScanner()
{
    work.reset();
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
    has_bad_fd = (socket_fd == -1);
    if (not has_bad_fd)
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

int TCPScanner::TCPClient::send(const std::vector<uint8_t>& packet) noexcept
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
    packet.resize(3000);

    int retval = select(socket_fd+1, &socket_set, nullptr, nullptr, &read_timeout);
    
    if (retval > 0)
    {
        int recv_size = ::recv(
            socket_fd,
            &packet[0],
            3000,
            0
        );
        if (recv_size > 0)
        {
            packet.resize(recv_size);
            return recv_size;
        }
        else if (recv_size < 0)
        {
            std::cout << "[TCP Scanner] Error recv: " << strerror(errno) << std::endl;
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

// int main()
// {
//     std::cout << "[TCP Scanner] TCP Scanner started\n";
//     TCPScanner scanner;
//     scanner.service_loop();
//     std::cout << "[TCP Scanner] TCP Scanner exit;\n";
// //     return 0;
// //     // boost::asio::ip::tcp::endpoint remote_server_(
// //     //     boost::asio::ip::address::from_string("8.8.4.4"),
// //     //     53
// //     // );
// //     // TCPScanner::TCPClient client("8.8.3.3");

// //     // client.connect();

// //     // if (not client.is_connected)
// //     // {
// //     //     std::cout << "client had not been connected" << std::endl;
// //     //     return 1;
// //     // }

// //     // std::string question = "nogizaka.yumi.ipl.eecs.case.edu";
// //     // std::vector<uint8_t> packet;

// //     // CRAFT_FULL_QUERY_TCP(question, packet)

// //     // std::vector<uint8_t> response;

// //     // int send_bytes;
// //     // int recv_bytes;
// //     // if ((send_bytes = client.send(packet)) < 0)
// //     // {
// //     //     std::cout << "send error" << std::endl;
// //     // }
// //     // else
// //     // {
// //     //     std::cout << "send bytes " << send_bytes << std::endl;
// //     // }
    

// //     // if ((recv_bytes = client.receive(response)) < 0)
// //     // {
// //     //     std::cout << "recv error" << std::endl;
// //     // }
// //     // else
// //     // {
// //     //     std::cout << "recv bytes " << recv_bytes << std::endl;
// //     // }

// //     // std::cout << "has a response of size " << response.size() << std::endl;

//     return 0;
// }
