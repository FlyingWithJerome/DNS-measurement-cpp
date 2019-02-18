#include "udp_scanner_listener.hpp"

constexpr uint16_t UDPListener::local_port_num;
constexpr uint16_t UDPListener::remote_port_num;

UDPListener::UDPListener(boost::asio::io_service& io_service)
: main_socket_(
    io_service,
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("0.0.0.0"), 
        2999
    )
)
{
    boost::interprocess::message_queue::remove("pipe_to_tcp");
    pipe_to_tcp_.reset(
        new boost::interprocess::message_queue(
            boost::interprocess::create_only,
            "pipe_to_tcp",
            1000,
            sizeof(message_pack)
        )
    );

    main_socket_.non_blocking(true);
    this->start_receive();
}


void UDPListener::start_receive()
{
    main_socket_.async_receive_from(
        boost::asio::null_buffers(),
        remote_endpoint_,
        boost::bind(
            &UDPListener::reactor_read, 
            this,
            boost::asio::placeholders::error
        )
    );
}

void UDPListener::reactor_read(const boost::system::error_code& error_code)
{
    if(error_code == boost::asio::error::operation_aborted)
        return;

    else if (not error_code)
    {
        boost::asio::ip::udp::endpoint sender_info;
        
        std::size_t available_packet_size = std::min((int)(main_socket_.available()), 1000);

        std::vector<uint8_t> new_arrival(available_packet_size);

        available_packet_size = main_socket_.receive_from(
            boost::asio::buffer(new_arrival),
            sender_info
        );

        new_arrival.resize(available_packet_size);

        handle_receive(new_arrival, sender_info);
    }
}

void UDPListener::handle_receive(
    const std::vector<uint8_t>& incoming_packet, 
    const boost::asio::ip::udp::endpoint& sender
)
{
    Tins::DNS incoming_response;
    try
    {
        incoming_response = Tins::DNS(incoming_packet.data(), incoming_packet.size());
    }
    catch(Tins::malformed_packet& except)
    {
        return;
    }

    if (incoming_response.rcode() == 0 and incoming_response.answers_count() > 0)
    { // is a legal response
        std::string question_name = incoming_response.answers()[0].dname();
        std::transform(question_name.begin(), question_name.end(), question_name.begin(), ::tolower);

        uint32_t question_id = NameTrick::get_question_id(question_name);

        // std::string response_status;
        // response_status = incoming_response.answers()[0].data() == "192.168.0.0" 
        // ? "ok" : "answer_error";

        // std::cout 
        // << "[Receiver] receive one from " 
        // << sender.address() 
        // << " with "
        // << response_status
        // << std::endl;

        NameTrick::JumboType jumbo_type_ = NameTrick::get_jumbo_type(question_name);
        
        if(jumbo_type_ == NameTrick::JumboType::no_jumbo) // not a jumbo query, will start a jumbo query
        {
            if (incoming_response.answers()[0].data() != "192.168.0.0")
            {
                // write down the answer error
                goto End;
            }
            std::vector<uint8_t> packet;
            std::string          hex_address;

            INT_TO_HEX(question_id, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = std::string("jumbo1-") 
            + hex_address 
            + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            CRAFT_FULL_QUERY_UDP(question, full_packet)

            main_socket_.async_send_to(
                boost::asio::buffer(packet),
                sender,
                boost::bind(
                    &UDPListener::handle_send,
                    this,
                    full_packet,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                )
            );
        }
        else
        {
            if (incoming_response.truncated()) // is a jumbo code and is truncated (as our expectation)
            {
                // send to tcp scanner
                message_pack outgoing;
                strcpy(outgoing.ip_address, sender.address().to_string().c_str());

                pipe_to_tcp_->send(&outgoing, sizeof(outgoing), 1);
            }
            else // is a jumbo query but is not truncated
            {
                // write down to [unable process fallback due to truncation]
            }
        }
    }
    else
    {
        // is not even a legal udp packet
    }

    End:
    start_receive();
}

void UDPListener::handle_send(std::vector<uint8_t>&, const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
