#ifndef UDPSERVER_
#define UDPSERVER_

#define FROM_UDP_SERVER

#include <mutex>

#include "server_common.hpp"
#include "../log/log_service.hpp"
#include "../packet/packet_factory.hpp"
#include "../packet/edns.hpp"
#include "../system/keyboard_interruption.hpp"
#include "../packet/dns_process_util.hpp"

class UDPServer
{
    public:
        UDPServer(boost::asio::io_service&);

        static constexpr uint32_t rcv_buf_size = 16000000;
        static constexpr uint32_t lru_size = 10000;
    
    private:
        void start_receive();
        void reactor_read(const boost::system::error_code&);
        void handle_receive(const buffer_type&, std::size_t, const boost::asio::ip::udp::endpoint&);
        void handle_send(const boost::system::error_code&, std::size_t);

        ResponseFactory response_factory;

        boost::asio::ip::udp::socket   main_socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;

        lru_cache response_count_cache;
};
#endif
