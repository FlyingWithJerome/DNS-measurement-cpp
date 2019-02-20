#ifndef UDP_SCANNER_LISTENER_
#define UDP_SCANNER_LISTENER_

#include <algorithm>
#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <tins/dns.h>

#include "../log/log_service.hpp"
#include "../packet/name_trick.hpp"
#include "query_commons.hpp"

class UDPListener
{
    public:
        UDPListener(boost::asio::io_service&);
        UDPListener(const UDPListener&) = delete;
        ~UDPListener();

        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;
    
    private:
        void start_receive();
        void reactor_read(const boost::system::error_code&);
        void handle_receive(const std::vector<uint8_t>&, const boost::asio::ip::udp::endpoint&);
        void handle_send(std::vector<uint8_t>&, const boost::system::error_code&, std::size_t);

        boost::asio::ip::udp::socket   main_socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;

        boost::scoped_ptr<boost::interprocess::message_queue> pipe_to_tcp_;

        typedef struct{
            char ip_address[17];
            char question[70];
            // char hex_form[12];
            // unsigned int ip_in_int;
        }message_pack;
};

#endif
