#ifndef TCPSCANNER_
#define TCPSCANNER_
#include <iostream>
#include <vector>
#include <thread>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

class TCPScanner
{
    public:
        TCPScanner();
        TCPScanner(const TCPScanner&) = delete;
        ~TCPScanner();
        int service_loop(boost::asio::io_service& io_service_);

    private:
        void perform_tcp_query(boost::asio::io_service& io_service_, const char* ip_address);

        std::vector<std::thread> thread_nest_;
        boost::scoped_ptr<boost::interprocess::message_queue> pipe_to_tcp_;

        typedef struct{
            char ip_address[17];
            // char hex_form[12];
            // unsigned int ip_in_int;
        }message_pack;
};

#endif
