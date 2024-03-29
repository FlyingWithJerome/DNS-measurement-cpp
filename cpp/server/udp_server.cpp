#include "udp_server.hpp"

constexpr uint32_t UDPServer::rcv_buf_size;
constexpr uint32_t UDPServer::lru_size;

UDPServer::UDPServer(boost::asio::io_service& io_service)
: main_socket_(
    io_service, 
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(SERVER_ADDRESS), 
        DNS_PORT
    )
)
, response_count_cache(
    UDPServer::lru_size
)
{

    boost::asio::socket_base::receive_buffer_size option(rcv_buf_size);
    main_socket_.set_option(option);

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Server] UDP socket buffer size " << buf_size.value() << std::endl;

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
        catch(KeyboardInterruption)
        {
            std::cout << "[UDP Server] Caught KeyboardInterruption, exiting from packet reading..." << std::endl;
            return;
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
    try
    {
        incoming_query = Tins::DNS(incoming_packet.data(), packet_size);
        const std::pair<std::string, int> name_type_pair = extract_name_type_pair(incoming_query);

        if (name_type_pair.second == INVALID_QUERY_TYPE)
        {
            goto End;
        }
        else
        {
            int number_of_entries = query_lru(response_count_cache, name_type_pair.first);
            if (number_of_entries != ENTRY_NOT_EXIST)
            {
                goto End;
            }
        }

        const std::string &question_name  = name_type_pair.first;
        const int &query_type = name_type_pair.second;
        const NameUtilities::QueryProperty query_property(question_name);

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
        {
            UDP_SERVER_STANDARD_LOG(query_property, sender, query_type)
        }
        else
        {
            UDP_SERVER_TRUNCATION_LOG(query_property, sender, query_type)
        }

        if (query_property.normal_query_over_tcp)
        {
            UDP_SERVER_SENDER_OVER_TCP_LOG(query_property, sender)
        }

        EDNS edns_result(incoming_query);
        UDP_SERVER_EDNS_LOG(query_property, sender, edns_result)
            
    }
    catch(KeyboardInterruption)
    {
        std::cout << "[UDP Server] Caught KeyboardInterruption, exiting from packet processing..." << std::endl;
        return;
    }
    catch(...)
    {
        std::cout << "[UDP Server] " << sender.address().to_string() << " had sent a malformed packet of size " << packet_size << std::endl;
        UDP_SERVER_MALFORM_PACKET_LOG(sender) 
    }

    End:
    start_receive();
}

void UDPServer::handle_send(const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
