#include "udp_server.hpp"

constexpr char UDPServer::udp_log_name_[];
constexpr char UDPServer::udp_tc_log_name_[];
constexpr char UDPServer::udp_edns_log_name_[];
constexpr char UDPServer::udp_malform_log_name[];
constexpr char UDPServer::sender_over_tcp_log_name[];

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

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Server] UDP socket buffer size " << buf_size.value() << std::endl;

    init_new_log_file(udp_log_name_);
    init_new_log_file(udp_tc_log_name_);
    init_new_log_file(udp_edns_log_name_);
    init_new_log_file(udp_malform_log_name);
    init_new_log_file(sender_over_tcp_log_name);

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

        try
        {
            buffer_type new_arrival(available_packet_size);

            available_packet_size = main_socket_.receive_from(
                boost::asio::buffer(
                    new_arrival
                ),
                sender_info
            );

            handle_receive(new_arrival, available_packet_size, sender_info);
        }
        catch(...)
        {
            start_receive();
        }
        
    }

    else
    {
        start_receive();
    }
    
}

void UDPServer::handle_receive(const buffer_type& incoming_packet, std::size_t packet_size, const boost::asio::ip::udp::endpoint& sender)
{
    Tins::DNS incoming_query;
    std::string question_name;

    try
    {
        incoming_query = Tins::DNS(incoming_packet.data(), packet_size);
        question_name  = incoming_query.queries()[0].dname();
        NameUtilities::QueryProperty query_property(question_name);

        std::cout 
        << "[UDP] Processing " 
        << question_name 
        << " <Question Identifier> " << query_property.question_id
        << " " << sender.address() 
        << ":" << sender.port()
        << " ac: " << (int)query_property.expect_answer_count
        << " an: " << (int)query_property.expect_number_of_answers
        << " <TR Flag>: " << query_property.will_truncate
        << "\n";

        std::vector<uint8_t> raw_data;

        response_factory.make_packet(
            PacketTypes::UDP_RESPONSE,
            incoming_query,
            query_property,
            raw_data
        );

        main_socket_.async_send_to(
            boost::asio::buffer(
                raw_data
            ), 
            sender,
            boost::bind(
                &UDPServer::handle_send, 
                this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            )
        );

        if (not query_property.will_truncate)
            UDP_SERVER_STANDARD_LOG(udp_log_name_, query_property, sender)

        else
            UDP_SERVER_TRUNCATION_LOG(udp_tc_log_name_, query_property, sender)

        if (query_property.normal_query_over_tcp)
            UDP_SERVER_SENDER_OVER_TCP_LOG(sender_over_tcp_log_name, query_property, sender)

        EDNS edns_result(incoming_query);
        UDP_SERVER_EDNS_LOG(udp_edns_log_name_, query_property, sender, edns_result)
    }
    catch(...)
    {
        std::cout << "[UDP Server] " << sender.address().to_string() << " had sent a malformed packet of size " << packet_size << std::endl;
        UDP_SERVER_MALFORM_PACKET_LOG(udp_malform_log_name, sender) 
    }

    start_receive();
}

void UDPServer::handle_send(const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
