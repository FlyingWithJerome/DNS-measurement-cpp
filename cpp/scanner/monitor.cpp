#include "monitor.hpp"

BufferMonitor::BufferMonitor(
    std::shared_ptr<msg_q> message_queue,
    boost::asio::ip::udp::socket& listening_socket
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

    float msg_q_percentage = 
    (float)(message_queue_->get_num_msg()) /
    msg_queue_size_;
    
    return socket_buffer_percentage < 0.3 and msg_q_percentage < 0.3;
}

void BufferMonitor::start_monitor()
{
    // while (true)
    // {
    //     if (not check_buffer())
    // }
}