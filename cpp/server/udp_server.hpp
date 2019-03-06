#ifndef UDPSERVER_
#define UDPSERVER_

#define FROM_UDP_SERVER

#include "server_common.hpp"
#include "../packet/edns.hpp"

class UDPServer
{
    public:
        UDPServer(boost::asio::io_service&);

        static constexpr char udp_log_name_[]        = "udp_normal_response.log";
        static constexpr char udp_tc_log_name_[]     = "udp_truncated_response.log";
        static constexpr char udp_edns_log_name_[]   = "udp_edns_record.log";
        static constexpr char udp_malform_log_name[] = "udp_malform_record.log";

        static constexpr uint32_t rcv_buf_size = 16000000;
    
    private:
        void start_receive();
        void reactor_read(const boost::system::error_code&);
        void handle_receive(const buffer_type&, std::size_t, const boost::asio::ip::udp::endpoint&);
        void handle_send(const boost::system::error_code&, std::size_t);

        boost::asio::ip::udp::socket   main_socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;

};
#endif