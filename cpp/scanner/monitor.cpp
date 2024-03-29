#include "monitor.hpp"

BufferMonitor::BufferMonitor(
    std::shared_ptr<msg_q> message_queue,
    boost::asio::ip::udp::socket& listening_socket,
    std::atomic<bool>& sender_should_wait,
    std::atomic<bool>& ddos_hold_on
)
: message_queue_(
    message_queue
)
, listening_socket_(
    listening_socket
)
, msg_queue_size_(
    message_queue->get_max_msg()
)
, sender_should_wait_(
    sender_should_wait
)
, is_stop(
    false
)
{
    boost::asio::socket_base::receive_buffer_size recv_size;
    listening_socket_.get_option(recv_size);

    size_t socket_buffer_size = recv_size.value();
    std::cout << "[Monitor] Successfully retrieved the recv buffer size: " << socket_buffer_size << "\n";
    socket_buffer_size_ = socket_buffer_size;
}

bool BufferMonitor::check_buffer()
{
    boost::asio::socket_base::bytes_readable buffer_occupied;
    listening_socket_.io_control(buffer_occupied);

    size_t buffer_used = buffer_occupied.get();

    float socket_buffer_percentage = 0.0;
    if (buffer_used)
    {
        socket_buffer_percentage = 
        (double)buffer_used /
        socket_buffer_size_;
    }

    if (message_queue_)
    {
        float msg_q_percentage = 
        (float)(message_queue_->get_num_msg()) /
        msg_queue_size_;

        return socket_buffer_percentage < 0.3 and msg_q_percentage < 0.3;
    }
    else
    {
        return true;
    }
}

void BufferMonitor::start_monitor()
{
    while (not is_stop)
    {
        try
        {
            sender_should_wait_ = not check_buffer();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
        catch(...)
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    }
}

void BufferMonitor::stop_monitor()
{
    is_stop = true;
}
