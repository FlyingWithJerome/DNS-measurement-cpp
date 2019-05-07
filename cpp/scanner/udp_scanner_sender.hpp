#ifndef UDP_SCANNER_SENDER_
#define UDP_SCANNER_SENDER_

#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include <time.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio/basic_raw_socket.hpp>
#include <boost/asio/raw_socket_service.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "token_bucket.hpp"
#include "../system/keyboard_interruption.hpp"
#include "../packet/packet_factory.hpp"

#define NANOSECONDS 1000000000L

bool is_public_ip_address(const uint32_t&);

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
            const std::string&,
            const std::uint32_t&,
            const std::uint32_t&,
            const float&,
            boost::asio::io_service&,
            std::shared_ptr<boost::interprocess::message_queue>&,
            std::atomic<bool>&,
            std::atomic<bool>&
        );

        ~UDPSender();

        int start_send() noexcept;
        int stop_send() noexcept;

        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;

        static constexpr uint16_t sleep_time       = 20;

        static constexpr uint32_t number_of_public_addresses = 3702258432;

    private:
        void handle_send(const boost::system::error_code&, std::size_t);

        std::ifstream file_input_;
        boost::asio::basic_raw_socket<raw> socket_;

        std::shared_ptr<boost::interprocess::message_queue> message_queue_;

        const uint32_t num_of_scanning_addr;

        uint32_t start_line_;
        uint32_t current_line_;
        uint32_t num_of_packets_sent;
        std::atomic<bool>& sender_wait_signal_;
        std::atomic<bool>& ddos_hold_on_signal_;

        QueryFactory packet_factory_;
        TokenBucket bucket_;
        boost::asio::deadline_timer queue_overflow_sleeper;

        struct timespec flow_control_sleep_;
        struct timespec flow_control_sleep_idle_;

        bool is_stop;
};

#endif
