#include "udp_scanner_sender.hpp"

constexpr uint16_t UDPSender::local_port_num;
constexpr uint16_t UDPSender::remote_port_num;
constexpr uint16_t UDPSender::sleep_time;
constexpr uint32_t UDPSender::number_of_public_addresses;

UDPSender::UDPSender(
    const std::string& input_file,
    const std::string& type_of_query,
    const std::uint32_t& send_rate,
    const float& percent_of_scan_address,
    boost::asio::io_service& io_service,
    std::shared_ptr<boost::interprocess::message_queue>& message_queue,
    std::atomic<bool>& wait_signal
)
: file_input_(
    input_file
)
, socket_(
    io_service
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
, queue_overflow_sleeper(
    io_service
)
, bucket_(
    send_rate, send_rate
)
, num_of_scanning_addr(
    static_cast<uint32_t>(percent_of_scan_address * number_of_public_addresses)
)
, type_of_query_(
    string_to_query_type(type_of_query)
)
{
    flow_control_sleep_.tv_nsec = NANOSECONDS / send_rate;
    flow_control_sleep_.tv_sec  = 0;

    socket_.open();
    socket_.non_blocking(true);

    std::cout << "[UDP Sender] number of addresses covered in this scan: " << num_of_scanning_addr << "\n";
    std::cout << "[UDP Sender] message queue size: " << message_queue_->get_max_msg() << "\n";
}

int UDPSender::start_send() noexcept
{
    std::string target;
    while(std::getline(file_input_, target) and num_of_packets_sent <= num_of_scanning_addr)
    {
        try
        {
            unsigned int address = std::stoul(target, nullptr, 10);
            std::string hex_address;

            INT_TO_HEX(address, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            packet_configuration packet_config_;
            packet_config_.id         = 1338;
            packet_config_.query_type = type_of_query_;
            packet_config_.q_name     = hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            packet_factory_.make_packet(
                PacketTypes::RAW_QUERY,
                packet_config_,
                full_packet
            );

            struct in_addr ip_address;
            ip_address.s_addr = __builtin_bswap32(address);

            raw::endpoint end_point(
                boost::asio::ip::address::from_string(inet_ntoa(ip_address)),
                UDPSender::remote_port_num
            );

            while(not bucket_.consume(1)) // flow control with token bucket
            {
                nanosleep(&flow_control_sleep_, &flow_control_sleep_idle_);
            }

            if (sender_wait_signal_)
            {
                boost::thread::id thread_id = boost::this_thread::get_id();
                std::cout << "[UDP Sender] going to sleep for " << sleep_time << "seconds (id:" << thread_id << ")\n";

                queue_overflow_sleeper.expires_from_now(boost::posix_time::seconds(sleep_time));
                queue_overflow_sleeper.wait();
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
            std::cerr << "[UDP Scanner] message from loop " << e.what() << '\n';
        }
        
    }
    std::cout << "[UDP Scanner] Total Packets Sent: " << num_of_packets_sent << std::endl;
    return 0;
}

void UDPSender::handle_send(const boost::system::error_code& error_code, std::size_t)
{
    if (error_code)
    {
        std::cout << error_code.message() << std::endl;
    }
}

