#include "udp_scanner_sender.hpp"

constexpr uint16_t UDPSender::local_port_num;
constexpr uint16_t UDPSender::remote_port_num;
constexpr uint16_t UDPSender::sleep_time;
constexpr uint32_t UDPSender::number_of_public_addresses;

UDPSender::UDPSender(
    const std::string& input_file,
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
, is_stop(
    false
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
    while((not is_stop) and std::getline(file_input_, target) and num_of_packets_sent <= num_of_scanning_addr)
    {
        try
        {
            unsigned int address = std::stoul(target, nullptr, 10);
            if (not is_public_ip_address(address))
            {
                continue;
            }

            std::string hex_address;

            INT_TO_HEX(address, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            packet_configuration packet_config_;
            packet_config_.id         = 1338;
            packet_config_.query_type = QueryType::A;
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
        catch(const KeyboardInterruption& e)
        {
            std::cout << "[UDP Scanner] UDP scanner is exiting...\n";
            return -1;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[UDP Scanner] message from loop " << e.what() << '\n';
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

int UDPSender::stop_send() noexcept
{
    is_stop = true;
}

UDPSender::~UDPSender()
{
    std::cout << "[UDP Scanner] Total Packets Sent: " << num_of_packets_sent << std::endl;
}

bool is_public_ip_address(const unsigned int& int_address)
{
    return
        !((0          <= int_address) &&  (int_address <= 16777215)) &&
        !((167772160  <= int_address) &&  (int_address <= 184549375)) &&
        !((1681915904 <= int_address) &&  (int_address <= 1686110207)) &&
        !((2130706432 <= int_address) &&  (int_address <= 2147483647)) &&
        !((2851995648 <= int_address) &&  (int_address <= 2852061183)) &&
        !((2886729728 <= int_address) &&  (int_address <= 2887778303)) &&
        !((3221225472 <= int_address) &&  (int_address <= 3221225727)) &&
        !((3221225984 <= int_address) &&  (int_address <= 3221226239)) &&
        !((3227017984 <= int_address) &&  (int_address <= 3227018239)) &&
        !((3232235520 <= int_address) &&  (int_address <= 3232301055)) &&
        !((3323068416 <= int_address) &&  (int_address <= 3323199487)) &&
        !((3325256704 <= int_address) &&  (int_address <= 3325256959)) &&
        !((3405803776 <= int_address) &&  (int_address <= 3405804031)) &&
        !((3758096384 <= int_address) &&  (int_address <= 4026531839)) &&
        !((4026531840 <= int_address) &&  (int_address <= 4294967295)) &&
        // !(int_address == 216816994) && // AT&T's
        !(int_address == 4294967295);
}

