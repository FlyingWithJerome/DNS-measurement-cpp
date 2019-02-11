#include "udp_server.hpp"

constexpr char UDPServer::udp_log_name_[];
constexpr char UDPServer::udp_tc_log_name_[];
constexpr char UDPServer::udp_edns_log_name_[];
constexpr char UDPServer::udp_malform_log_name[];

UDPServer::UDPServer(boost::asio::io_service& io_service)
:main_socket_(
    io_service, 
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(SERVER_ADDRESS), 
        DNS_PORT
    )
)
{    
    boost::asio::socket_base::send_buffer_size buf_size;

    main_socket_.get_option(buf_size);

    std::cout << "udp socket buffer size " << buf_size.value() << std::endl;

    init_new_log_file(udp_log_name_);
    init_new_log_file(udp_tc_log_name_);
    init_new_log_file(udp_edns_log_name_);

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
    try
    {
        incoming_query = Tins::DNS(incoming_packet.get(), packet_size);
    }
    catch(Tins::malformed_packet& except)
    {
        MALFORM_PACKET_UDP_LOG(udp_malform_log_name, sender)
        return;
    }

    NameTrick::QueryProperty query_property(incoming_query.queries()[0].dname());

    buffer_type write_buffer = NULL;

    std::cout 
    << "[UDP] Processing " 
    << query_property.name 
    << " <Question Identifier> " << query_property.question_id
    << " " << sender.address() 
    << ":" << sender.port() 
    << " <TR Flag>: " << query_property.will_truncate
    << std::endl;

    if (not query_property.will_truncate)
        UDP_STANDARD_LOG(udp_log_name_, query_property, sender)

    else
        UDP_TRUNCATION_LOG(udp_tc_log_name_, query_property, sender)

    RESPONSE_MAKER_UDP(incoming_query, query_property, write_buffer)

    // size_t attempted_out_size = ResponseMaker::response_maker_udp(
    //     incoming_query,
    //     query_property,
    //     write_buffer
    // );

    main_socket_.async_send_to(
        boost::asio::buffer(
            raw_data
            // write_buffer.get(),
            // attempted_out_size
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
    << std::endl;

    start_receive();
}

void UDPServer::handle_send(buffer_type&, const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
