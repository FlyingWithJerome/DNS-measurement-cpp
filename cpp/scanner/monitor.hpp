#ifndef MONITOR_
#define MONITOR_

#include <memory>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

typedef boost::interprocess::message_queue msg_q;

class BufferMonitor
{
    public:
        BufferMonitor(
            std::shared_ptr<msg_q>,
            boost::asio::ip::udp::socket&
        );
        BufferMonitor(const BufferMonitor&) = delete;

        void start_monitor();

    private:
        bool check_buffer();

        std::shared_ptr<msg_q> message_queue_;
        boost::asio::ip::udp::socket& listening_socket_;

        std::uint32_t socket_buffer_size_;
        std::size_t   msg_queue_size_;
};

#endif
