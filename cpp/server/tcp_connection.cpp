#include "tcp_connection.hpp"

TCPConnection::TCPConnection(
    const ResponseFactory& response_factory, 
    boost::asio::io_service& io_service
)
: main_socket_(
    io_service
)
, response_factory_(
    response_factory
)
{
}

TCPConnection::pointer TCPConnection::create(
    const ResponseFactory& response_factory,
    boost::asio::io_service& io_service
)
{
    return pointer(new TCPConnection(response_factory, io_service));
}

boost::asio::ip::tcp::socket& TCPConnection::socket()
{
    return main_socket_;
}

void TCPConnection::start()
{    
    main_socket_.async_read_some(
        boost::asio::buffer(buffer_, 1000),
        boost::bind(
            &TCPConnection::handle_reactor_receive,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        )
    );
}


void TCPConnection::handle_reactor_receive(
    const boost::system::error_code& receive_error, 
    std::size_t read_size
)
{
    boost::system::error_code endpoint_error;
    boost::asio::ip::tcp::endpoint remote_endpoint = main_socket_.remote_endpoint(endpoint_error);

    if (receive_error or endpoint_error) // error in receiving or resolving the remote endpoint
    {
        std::cout << receive_error.message() << " | " << endpoint_error.message() << std::endl;
        return;
    }
    else if (read_size <= 2) // not a proper size of a DNS packet
    {
        TCP_SERVER_MALFORM_LOG(remote_endpoint)        
        return;
    }

    Tins::DNS readable_packet;

    try
    {
        readable_packet = Tins::DNS(buffer_+2, read_size-2);
    }
    catch(...)
    {
        TCP_SERVER_MALFORM_LOG(remote_endpoint)  
        return;
    }

    try
    {
        if (readable_packet.queries().size() > 0)
        {
            const NameUtilities::QueryProperty query_property(readable_packet.queries()[0].dname());
            const int query_type = static_cast<int>(readable_packet.queries()[0].query_type());

            std::vector<uint8_t> raw_data;

            response_factory_.make_packet(
                PacketTypes::TCP_RESPONSE,
                readable_packet,
                query_property,
                raw_data
            );

            main_socket_.async_send(
                boost::asio::buffer(raw_data), 
                boost::bind(
                    &TCPConnection::handle_send, 
                    shared_from_this(), 
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                )
            );

            TCP_SERVER_STANDARD_LOG(query_property, main_socket_, query_type)
        }
        else
        {
            TCP_SERVER_MALFORM_LOG(remote_endpoint)
        }
    }
    catch(...)
    {
    }
}

void TCPConnection::handle_send(
    const boost::system::error_code& error_code, 
    std::size_t
)
{
    if (error_code)
        std::cout << error_code.message() << std::endl;
}
