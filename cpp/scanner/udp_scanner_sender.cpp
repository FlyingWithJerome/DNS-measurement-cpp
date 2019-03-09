#include "udp_scanner_sender.hpp"

constexpr uint16_t UDPSender::local_port_num;
constexpr uint16_t UDPSender::remote_port_num;
constexpr uint64_t UDPSender::packet_send_rate;
constexpr uint64_t UDPSender::sleep_per_iter;
constexpr uint16_t UDPSender::sleep_time;

UDPSender::UDPSender(
    boost::asio::io_service& io_service,
    std::shared_ptr<boost::interprocess::message_queue>& message_queue,
    const std::string& input_file,
    std::atomic<bool>& wait_signal
)
: file_input_(
    input_file
)
, socket_(
    io_service
)
, bucket_(
    packet_send_rate
)
, message_queue_(
    message_queue
)
, num_of_packets_sent(
    0
)
, sender_wait_signal_(
    wait_signal
)
{
    socket_.open();
    socket_.non_blocking(true);

    std::cout << "[UDP Sender] message queue size: " << message_queue_->get_max_msg() << "\n";
}

int UDPSender::start_send() noexcept
{
    std::string target;
    while(std::getline(file_input_, target))
    {
        try
        {
            unsigned int address = std::stoul(target, nullptr, 10);
            std::string hex_address;

            INT_TO_HEX(address, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            CRAFT_FULL_QUERY_RAW(question, full_packet)

            struct in_addr ip_address;
            ip_address.s_addr = __builtin_bswap32(address);

            raw::endpoint end_point(
                boost::asio::ip::address::from_string(inet_ntoa(ip_address)),
                UDPSender::remote_port_num
            );

            std::cout << "[UDP Sender] send to " << inet_ntoa(ip_address) << "\n";
            bucket_.consume_one_packet();

            // socket_.async_send_to(
            //     boost::asio::buffer(full_packet),
            //     end_point,
            //     boost::bind(
            //         &UDPSender::handle_send,
            //         this,
            //         boost::asio::placeholders::error,
            //         boost::asio::placeholders::bytes_transferred
            //     )
            // );

            if (sender_wait_signal_)
            {
                boost::thread::id thread_id = boost::this_thread::get_id();
                std::cout << "[UDP Sender] going to sleep for " << sleep_time << "seconds (id:" << thread_id << ")\n";

                boost::asio::deadline_timer timer(socket_.get_io_service());
                timer.expires_from_now(boost::posix_time::seconds(sleep_time));
                timer.wait();
                std::cout << "[UDP Sender] sender waked up (id:" << thread_id << ")\n";
            }
            
            socket_.send_to(
                boost::asio::buffer(full_packet),
                end_point
            );
            num_of_packets_sent++;
        }
        catch(const std::exception& e)
        {
            std::cerr << "message from loop " << e.what() << '\n';
        }
        
    }
    return 0;
}

void UDPSender::handle_send(const boost::system::error_code& error_code, std::size_t)
{
    if (error_code)
    {
        std::cout << error_code.message() << std::endl;
    }
}

