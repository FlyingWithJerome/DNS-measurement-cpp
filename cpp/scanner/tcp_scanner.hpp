#ifndef TCPSCANNER_
#define TCPSCANNER_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <iostream>
#include <vector>
#include <thread>
#include <mutex> 

#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/bind.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/system/system_error.hpp>

#include <tins/dns.h>

#include "query_commons.hpp"

class TCPScanner
{
    public:
        TCPScanner();
        TCPScanner(const TCPScanner&) = delete;
        ~TCPScanner();
        int service_loop();

    private:
        void perform_tcp_query(
            std::string,
            std::string
        );

        std::vector<std::thread> thread_nest_;
        boost::scoped_ptr<boost::interprocess::message_queue> pipe_to_tcp_;

        typedef struct{
            char ip_address[17];
            char question[70];
            // char hex_form[12];
            // unsigned int ip_in_int;
        }message_pack;

        std::mutex mutex_;

    public:
        class TCPClient
        {
            public:
                TCPClient(
                    const char* remote_addr
                );
                TCPClient(const TCPClient&) = delete;

                ~TCPClient();

                int connect();
                int send(const std::vector<uint8_t>&);
                int receive(std::vector<uint8_t>&);

                bool is_connected;

            private:
                int socket_fd;
                struct sockaddr_in remote_address;

                fd_set socket_set;

                struct timeval connect_timeout = {3, 0};
                struct timeval read_timeout    = {8, 0};
                // void handle_wait(const boost::system::error_code&);
                // void handle_connect(const boost::system::error_code&);

                // void check_deadline();

                // boost::asio::io_service io_service_;
                // boost::asio::ip::tcp::endpoint remote_endpoint_;
                // boost::asio::ip::tcp::socket   socket_;
                // boost::asio::deadline_timer deadline_;

                // bool timeout;
        };
};

#endif
