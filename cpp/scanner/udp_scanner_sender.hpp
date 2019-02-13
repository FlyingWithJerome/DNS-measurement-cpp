#ifndef UDP_SCANNER_SENDER_
#define UDP_SCANNER_SENDER_

#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "query_commons.hpp"

class UDPSender
{
    public:
        UDPSender(const std::string&, const boost::asio::io_service&);
        ~UDPSender();

        static constexpr uint16_t local_port_num  = 2999;
        static constexpr uint16_t remote_port_num = 53;

    private:
        int start_send();
        void handle_send(const boost::system::error_code& error_code, std::size_t);

        std::istream file_input_;
        boost::asio::basic_raw_socket<asio::ip::raw> socket_;

        char udp_header[8];
};

#endif
