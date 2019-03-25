#ifndef TCPCONNECTION_
#define TCPCONNECTION_

#include <memory>
#include <boost/enable_shared_from_this.hpp>

#define FROM_TCP_SERVER
#include "server_common.hpp"
#include "../packet/packet_factory.hpp"
#include "../log/log_service.hpp"

class TCPConnection : public boost::enable_shared_from_this<TCPConnection>
{
    public:
        typedef boost::shared_ptr<TCPConnection> pointer;

        static pointer create(
            const ResponseFactory&,
            boost::asio::io_service&
        );

        boost::asio::ip::tcp::socket& socket();

        void start();

    private:
        TCPConnection(
            const ResponseFactory&,
            boost::asio::io_service&
        );
        
        void handle_receive(
            const buffer_type&,
            std::size_t
        );

        void handle_reactor_receive(
            const boost::system::error_code&, 
            std::size_t
        );

        void handle_send(
            const boost::system::error_code&, 
            std::size_t
        );

        boost::asio::ip::tcp::socket main_socket_;
        const ResponseFactory& response_factory_;

        uint8_t buffer_[1000];        
};
#endif 
