#ifndef UDP_SCANNER_LISTENER_
#define UDP_SCANNER_LISTENER_

#include <algorithm>
#include <vector>
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <tins/dns.h>

#include "../log/log_service.hpp"
#include "../packet/name_util.hpp"
#include "../packet/packet_factory.hpp"
#include "message_queue_packet.hpp"

#define SEND_TO_TCP_SCANNER(q_name, q_type) message_pack outgoing; \
    strcpy(outgoing.ip_address, sender.address().to_string().c_str()); \
    strcpy(outgoing.question,   (q_name).c_str()); \
    outgoing.query_type=static_cast<int>(q_type);  \
    pipe_to_tcp_->send(&outgoing, sizeof(outgoing), 1);

#define SEND_OUT_PACKET(alias, question_name, question_type, sender) packet_configuration packet_config_##alias; \
    packet_config_##alias.id     = 1338; \
    packet_config_##alias.q_name = question_name; \
    packet_config_##alias.query_type = question_type; \
    std::vector<uint8_t> packet_carrier_##alias; \
    packet_factory_.make_packet( \
        PacketTypes::UDP_QUERY, \
        packet_config_##alias, \
        packet_carrier_##alias \
    ); \
    main_socket_.async_send_to( \
        boost::asio::buffer(packet_carrier_##alias), \
        sender, \
        boost::bind( \
            &UDPListener::handle_send, \
            this, \
            boost::asio::placeholders::error, \
            boost::asio::placeholders::bytes_transferred \
        ) \
    );

class UDPListener
{
    public:
        UDPListener(boost::asio::io_service&, std::shared_ptr<boost::interprocess::message_queue>&);
        UDPListener(const UDPListener&) = delete;
        ~UDPListener();

        boost::asio::ip::udp::socket& get_socket();

        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;

        static constexpr uint32_t rcv_buf_size = 16 * 1000 * 1000; //16 mega bytes, BIND9 uses this size
    
    private:
        void start_receive();
        void reactor_read(const boost::system::error_code&);
        void handle_receive(const std::vector<uint8_t>&, const boost::asio::ip::udp::endpoint&);
        void handle_send(const boost::system::error_code&, std::size_t);

        QueryFactory packet_factory_;

        boost::asio::ip::udp::socket   main_socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;

        std::shared_ptr<boost::interprocess::message_queue> pipe_to_tcp_;
};

#endif
