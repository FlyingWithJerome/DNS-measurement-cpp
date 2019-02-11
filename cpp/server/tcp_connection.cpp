#include "tcp_connection.hpp"

TCPConnection::TCPConnection(
    boost::asio::io_service& io_service,
    const char* file_name
)
:main_socket_(io_service)
,file_name_(file_name)
{
}

TCPConnection::pointer TCPConnection::create(
    boost::asio::io_service& io_service,
    const char* file_name
)
{
    return pointer(new TCPConnection(io_service, file_name));
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
    const boost::system::error_code& error, 
    std::size_t read_size
)
{
    if (error)
    {
        std::cout << error.message() << std::endl;
        return;
    }

    Tins::DNS readable_packet(buffer_+2, read_size-2);

    NameTrick::QueryProperty query_property(readable_packet.queries()[0].dname());

    buffer_type write_buffer = NULL;

    // size_t attempted_out_size = ResponseMaker::response_maker_tcp(
    //     readable_packet,
    //     query_property,
    //     write_buffer
    // );

    RESPONSE_MAKER_TCP(readable_packet, query_property, write_buffer)

    main_socket_.async_send(
        boost::asio::buffer(
            raw_data
            // write_buffer.get(),
            // attempted_out_size
        ), 
        boost::bind(
            &TCPConnection::handle_send, 
            shared_from_this(), 
            write_buffer,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        )
    );

    TCP_STANDARD_LOG(file_name_, query_property, main_socket_)
}

void TCPConnection::handle_send(
    buffer_type&, 
    const boost::system::error_code& error_code, 
    std::size_t
)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}
