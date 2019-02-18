#include "udp_scanner_sender.hpp"

constexpr uint16_t UDPSender::local_port_num;
constexpr uint16_t UDPSender::remote_port_num;

UDPSender::UDPSender(const std::string& input_file, boost::asio::io_service& io_service)
:file_input_(input_file)
,socket_(io_service)
{
    socket_.open();
    socket_.non_blocking(true);

}

int UDPSender::start_send()
{
    std::string target;
    while(std::getline(file_input_, target))
    {
        std::cout << "the line is " << target << std::endl;
        try
        {
            unsigned int address = std::stoul(target, nullptr, 10);
            std::string hex_address;

            INT_TO_HEX(address, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            CRAFT_FULL_QUERY_RAW(question, full_packet)

            std::cout << "full packet size " << full_packet.size() << std::endl;
            std::cout << "first byte of the full packet " << (int)(full_packet[0]) << std::endl;

            struct in_addr ip_address;
            ip_address.s_addr = __builtin_bswap32(address);

            raw::endpoint end_point(
                boost::asio::ip::address::from_string(inet_ntoa(ip_address)),
                UDPSender::remote_port_num
            );

            std::cout << "[Sender] send to " << inet_ntoa(ip_address) << std::endl;

            socket_.async_send_to(
                boost::asio::buffer(full_packet),
                end_point,
                boost::bind(
                    &UDPSender::handle_send,
                    this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                )
            );
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

