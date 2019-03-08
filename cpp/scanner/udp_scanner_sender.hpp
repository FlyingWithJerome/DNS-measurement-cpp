#ifndef UDP_SCANNER_SENDER_
#define UDP_SCANNER_SENDER_

#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio/basic_raw_socket.hpp>
#include <boost/asio/raw_socket_service.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>


#include <tins/dns.h>

#include "query_commons.hpp"
#include "token_bucket.hpp"

class raw
{
    public:
        typedef boost::asio::ip::basic_endpoint<raw> endpoint;
        static raw v4()
        {
            return raw(IPPROTO_UDP, AF_INET);
        }

        static raw v6()
        {
            return raw(IPPROTO_UDP, AF_INET);
        }

        explicit raw()
        : protocol_(IPPROTO_UDP)
        , family_(AF_INET)
        {
        }

        int type() const noexcept
        {
            return SOCK_RAW;
        }

        int protocol() const noexcept
        {
            return protocol_;
        }

        int family() const noexcept
        {
            return family_;
        }

        typedef boost::asio::basic_raw_socket<raw> socket;
        typedef boost::asio::ip::basic_resolver<raw> resolver;

        friend bool operator==(const raw& p1, const raw& p2) noexcept
        {
            return p1.protocol_ == p2.protocol_ && p1.family_ == p2.family_;
        }

        friend bool operator!=(const raw& p1, const raw& p2) noexcept
        {
            return p1.protocol_ != p2.protocol_ || p1.family_ != p2.family_;
        }

        explicit raw(int protocol_id, int protocol_family)
            : protocol_(protocol_id)
            , family_(protocol_family)
        {
        }
    private:
        int protocol_;
        int family_;
};

class UDPSender
{
    public:
        UDPSender(
            boost::asio::io_service&,
            std::shared_ptr<boost::interprocess::message_queue>&,
            const std::string&
        );

        int start_send() noexcept;

        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;

        static constexpr uint64_t packet_send_rate = 40000;

        static constexpr uint64_t sleep_per_iter   = 200*1000;
        static constexpr uint16_t sleep_time       = 20;

    private:
        void handle_send(const boost::system::error_code&, std::size_t);

        std::ifstream file_input_;
        boost::asio::basic_raw_socket<raw> socket_;

        std::shared_ptr<boost::interprocess::message_queue> message_queue_;

        uint32_t num_of_packets_sent;

        TokenBucket bucket_;
};

#endif
