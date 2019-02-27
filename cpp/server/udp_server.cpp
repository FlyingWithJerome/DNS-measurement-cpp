#include "udp_server.hpp"

constexpr char UDPServer::udp_log_name_[];
constexpr char UDPServer::udp_tc_log_name_[];
constexpr char UDPServer::udp_edns_log_name_[];
constexpr char UDPServer::udp_malform_log_name[];

constexpr uint32_t UDPServer::rcv_buf_size;

UDPServer::UDPServer(boost::asio::io_service& io_service)
:main_socket_(
    io_service, 
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(SERVER_ADDRESS), 
        DNS_PORT
    )
)
{    
    boost::asio::socket_base::receive_buffer_size option(rcv_buf_size);
    main_socket_.set_option(option);

    // int socket_fd = main_socket_.native_handle();

    // if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &UDPServer::rcv_buf_size, sizeof(uint32_t)) == -1) 
    // {
    //     std::cout << "[UDP Server] Error setting the rcv buf\n";
    // }

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Server] UDP socket buffer size " << buf_size.value() << std::endl;

    init_new_log_file(udp_log_name_);
    init_new_log_file(udp_tc_log_name_);
    init_new_log_file(udp_edns_log_name_);
    init_new_log_file(udp_malform_log_name);

    main_socket_.non_blocking(true);

    this->start_receive();
}


void UDPServer::start_receive()
{
    main_socket_.async_receive_from(
        boost::asio::null_buffers(),
        remote_endpoint_,
        boost::bind(
            &UDPServer::reactor_read, 
            this,
            boost::asio::placeholders::error
        )
    );
}

void UDPServer::reactor_read(const boost::system::error_code& error_code)
{
    if(error_code == boost::asio::error::operation_aborted)
        return;

    else if (not error_code)
    {
        boost::asio::ip::udp::endpoint sender_info;
        
        std::size_t available_packet_size = std::min((int)(main_socket_.available()), MAX_SIZE_PACKET_ACCEPT);

        buffer_type new_arrival(new uint8_t[available_packet_size]);

        available_packet_size = main_socket_.receive_from(
            boost::asio::buffer(
                new_arrival.get(), 
                available_packet_size
            ),
            sender_info
        );

        handle_receive(new_arrival, available_packet_size, sender_info);
    }
}

void UDPServer::handle_receive(const buffer_type& incoming_packet, std::size_t packet_size, const boost::asio::ip::udp::endpoint& sender)
{
    Tins::DNS incoming_query;
    std::string question_name;

    try
    {
        incoming_query = Tins::DNS(incoming_packet.get(), packet_size);
        question_name  = incoming_query.queries()[0].dname();
        NameTrick::QueryProperty query_property(question_name);

        buffer_type write_buffer = NULL;

        std::cout 
        << "[UDP] Processing " 
        << question_name 
        << " <Question Identifier> " << query_property.question_id
        << " " << sender.address() 
        << ":" << sender.port() 
        << " <TR Flag>: " << query_property.will_truncate
        << "\n";

        if (not query_property.will_truncate)
            UDP_STANDARD_LOG(udp_log_name_, query_property, sender)

        else
            UDP_TRUNCATION_LOG(udp_tc_log_name_, query_property, sender)

        RESPONSE_MAKER_UDP(incoming_query, query_property)

        main_socket_.async_send_to(
            boost::asio::buffer(
                raw_data
            ), 
            sender,
            boost::bind(
                &UDPServer::handle_send, 
                this, 
                write_buffer,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            )
        );

        EDNS edns_result(incoming_query);
        EDNS_LOG(udp_edns_log_name_, query_property, sender, edns_result)

        std::cout 
        << "the UDP Payload is " << edns_result.EDNS0_payload 
        << " the ECS subnet is " << edns_result.ECS_subnet_address 
        << "/" << edns_result.ECS_subnet_mask 
        << "\n";

    }
    catch(...)
    {
        std::cout << "[UDP Server] " << sender.address().to_string() << " had sent a malformed packet of size " << packet_size << std::endl;
        UDP_MALFORM_PACKET_LOG(udp_malform_log_name, sender) 
    }

    start_receive();
}

void UDPServer::handle_send(buffer_type&, const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
