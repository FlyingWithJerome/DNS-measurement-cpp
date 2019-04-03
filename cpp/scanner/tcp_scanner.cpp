#include "tcp_scanner.hpp"

constexpr size_t TCPScanner::largest_tcp_response_size;

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
                if (empty.query_type != -1)
                {
                    io_service_.post(
                        boost::bind(
                            &TCPScanner::perform_tcp_query,
                            this,
                            std::string(empty.ip_address),
                            std::string(empty.question),
                            empty.query_type
                        )
                    );
                }
                else
                {
                    break;
                }
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
    const std::string& question,
    const int& query_type
)
{
    TCPClient client(ip_address.c_str());
    NameUtilities::QueryProperty query_property(question);

    if (client.has_bad_fd)
    {
        return;
    }

    if (not client.connect())
    {
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "connect_timeout")
        return;
    }
    
    std::vector<uint8_t> full_packet;
    std::vector<uint8_t> response_packet;

    packet_configuration packet_config;
    packet_config.id         = 1338;
    packet_config.query_type = static_cast<QueryType>(query_type);
    packet_config.q_name     = question;

    packet_factory_.make_packet(
        PacketTypes::TCP_QUERY,
        packet_config,
        full_packet
    );

    if (client.send(full_packet) <= 0)
    {
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "fail_to_send")
        return;
    }
    
    if (client.receive(response_packet) <= 0)
    {
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "recv_timeout")
        return;
    }

    if ( response_packet.size() < 2 )
    {
        // write to malformed packet
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "receive_less_than_2_bytes")
        return;
    }

    Tins::DNS readable_response;

    try
    {
        readable_response = Tins::DNS(response_packet.data()+2, response_packet.size()-2);
    }
    catch (...)
    {
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "un-parsable_packet")
        return;
    }
    if (readable_response.answers_count() > 0)
    {
        std::cout << "[TCP Scanner] packet name: " << readable_response.answers()[0].dname() << "\n";

        std::string result;
        inspect_response(readable_response, query_property, result);

        TCP_SCANNER_NORMAL_LOG(
            ip_address, 
            query_property,
            query_type,
            readable_response.rcode(), 
            result
        )
    }
    else
    {
        TCP_SCANNER_MALFORMED_LOG(ip_address.c_str(), query_property, query_type, "zero_answer_count")
    }
}

void TCPScanner::inspect_response(
    const Tins::DNS& response, 
    const NameUtilities::QueryProperty& query_property, 
    std::string& result_str
) noexcept
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
    const QueryType query_type = static_cast<QueryType>(response.answers()[0].query_type());
    const int number_of_answers = response.answers_count();

    if( (number_of_answers == 1  and query_type == Tins::DNS::QueryType::TXT)
        or
        (number_of_answers == 50 and query_type == Tins::DNS::QueryType::A)
        or
        (number_of_answers == 26 and query_type == Tins::DNS::QueryType::MX)
        or
        (number_of_answers == 26 and query_type == Tins::DNS::QueryType::NS)
        or
        query_property.normal_query_over_tcp
    )
    {
        result_str = "answer_ok";
        return;
    }

    else
    {
        std::stringstream result_builder;
        result_builder 
        << "wrong_number_of_entries("
        << response.answers().size()
        << "/100)";

        result_str = result_builder.str();
        return;
    }
}

TCPScanner::~TCPScanner()
{
    std::cout << "[TCP Scanner] Going to exit, Wait for unfinished jobs...\n";
    work.reset();
    io_service_.stop();
    thread_pool_.join_all();
    std::cout << "[TCP Scanner] TCP scanner successfully exited\n";
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

int TCPScanner::TCPClient::connect() noexcept
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
    int status = ::send(socket_fd, packet.data(), packet.size(), 0);

    if (status < 0)
        std::cout << "[Send function] " << strerror(errno) << std::endl;        

    return status;
}

int TCPScanner::TCPClient::receive(std::vector<uint8_t>& packet)
{
    packet.resize(TCPScanner::largest_tcp_response_size);

    int retval = select(socket_fd+1, &socket_set, nullptr, nullptr, &read_timeout);
    
    if (retval > 0)
    {
        int recv_size = ::recv(socket_fd, &packet[0], TCPScanner::largest_tcp_response_size, 0);

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

