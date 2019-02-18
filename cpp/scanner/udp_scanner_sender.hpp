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
#include <boost/asio/basic_raw_socket.hpp>
#include <boost/asio/raw_socket_service.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>

#include <tins/dns.h>

#include "query_commons.hpp"

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

        int type() const
        {
            return SOCK_RAW;
        }

        int protocol() const
        {
            return protocol_;
        }

        int family() const
        {
            return family_;
        }

        typedef boost::asio::basic_raw_socket<raw> socket;
        typedef boost::asio::ip::basic_resolver<raw> resolver;

        friend bool operator==(const raw& p1, const raw& p2)
        {
            return p1.protocol_ == p2.protocol_ && p1.family_ == p2.family_;
        }

        friend bool operator!=(const raw& p1, const raw& p2)
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
        UDPSender(const std::string&, boost::asio::io_service&);
        
        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;

    private:
        int start_send();
        void handle_send(const boost::system::error_code&, std::size_t);

        std::ifstream file_input_;
        boost::asio::basic_raw_socket<raw> socket_;
};

#endif
