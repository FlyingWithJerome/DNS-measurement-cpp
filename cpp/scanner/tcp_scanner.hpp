#ifndef TCPSCANNER_
#define TCPSCANNER_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <iostream>
#include <vector>
#include <thread>
#include <mutex> 
#include <future>

#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/system/system_error.hpp>

#include <tins/dns.h>

#include "../log/log_service.hpp"
#include "../packet/name_trick.hpp"
#include "query_commons.hpp"

class TCPScanner
{
    public:
        TCPScanner();
        TCPScanner(const TCPScanner&) = delete;
        ~TCPScanner();
        int service_loop() noexcept;

        static constexpr char tcp_normal_log_[] = "tcp_scanner_normal.log";

    private:
        void perform_tcp_query(
            std::string,
            std::string
        );

        void inspect_response(const Tins::DNS&, std::string&);

        std::vector<std::thread> thread_nest_;
        boost::scoped_ptr<boost::interprocess::message_queue> pipe_to_tcp_;

        typedef struct{
            char ip_address[17];
            char question[70];
            // char hex_form[12];
            // unsigned int ip_in_int;
        }message_pack;

        boost::asio::io_service io_service_;
        boost::thread_group     thread_pool_;
        boost::scoped_ptr<boost::asio::io_service::work> work;

        class TCPClient
        {
            public:
                TCPClient(
                    const char* remote_addr
                );
                TCPClient(const TCPClient&) = delete;

                ~TCPClient();

                int connect();
                int send(const std::vector<uint8_t>&) noexcept;
                int receive(std::vector<uint8_t>&);

                bool is_connected;
                bool has_bad_fd;

            private:
                int socket_fd;
                struct sockaddr_in remote_address;

                fd_set socket_set;

                struct timeval connect_timeout = {3, 0};
                struct timeval read_timeout    = {8, 0};
        };
};

#endif
