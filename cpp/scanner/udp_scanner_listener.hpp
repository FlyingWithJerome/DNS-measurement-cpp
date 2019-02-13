#ifndef UDP_SCANNER_LISTENER_
#define UDP_SCANNER_LISTENER_

#include <boost/asio.hpp>

#include "../log/log_service.hpp"
#include "../packet/name_trick.hpp"
#include "query_commons.hpp"

class UDPListener
{
    public:
        UDPListener(boost::asio::io_service&);
    
    private:
        void start_receive();
        void reactor_read(const boost::system::error_code&);
        void handle_receive(const buffer_type&, std::size_t, const boost::asio::ip::udp::endpoint&);
        void handle_send(buffer_type&, const boost::system::error_code&, std::size_t);

        boost::asio::ip::udp::socket   main_socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;
};

#endif
