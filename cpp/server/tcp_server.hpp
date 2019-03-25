#ifndef TCPSERVER_
#define TCPSERVER_

#include "server_common.hpp"
#include "tcp_connection.hpp"
#include "../packet/packet_factory.hpp"

class TCPServer
{
    public:
        TCPServer(boost::asio::io_service&);

    private:
        void start_accept();

        void handle_accept(
            TCPConnection::pointer,
            const boost::system::error_code&
        );

        boost::asio::ip::tcp::acceptor acceptor_;
        ResponseFactory response_factory_;
};
#endif
