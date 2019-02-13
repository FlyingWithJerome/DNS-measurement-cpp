#include "udp_scanner_listener.hpp"

UDPListener::UDPListener(boost::asio::io_service& io_service)
:main_socket_(
    io_service,
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("192.5.110.81"), 
        2999
    )
)
{
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

        std::shared_ptr<uint8_t> new_arrival(new uint8_t[available_packet_size]);

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

void UDPServer::handle_receive(
    const std::shared_ptr<uint8_t>& incoming_packet, 
    std::size_t packet_size, 
    const boost::asio::ip::udp::endpoint& sender
)
{
    Tins::DNS incoming_response;
    try
    {
        incoming_response = Tins::DNS(incoming_packet.get(), packet_size);
    }
    catch(Tins::malformed_packet& except)
    {
        return;
    }

    if (incoming_response.rcode == 0 and incoming_response.answers_count() > 0)
    { // is a legal response
        uint32_t question_id = NameTrick::get_question_id(incoming_response.answers[0].dname());
        std::string response_status;
        response_status = incoming_response.answers[0].data() == "192.168.0.0" 
        ? "ok" : "answer_error";
    }
    else
    {

    }

    start_receive();
}

void UDPServer::handle_send(std::shared_ptr<uint8_t>&, const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
