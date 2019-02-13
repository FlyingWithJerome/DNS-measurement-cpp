#include "udp_scanner_sender.cpp"

constexpr uint16_t UDPSender::local_port_num;
constexpr uint16_t UDPSender::remote_port_num;

UDPSender::UDPSender(const std::string& input_file, const boost::asio::io_service& io_service)
:file_input(input_file)
,socket_(io_service)
{
    socket_.non_blocking(true);
    socket_.open();
}

int UDPSender::start_send()
{
    std::string address;
    while(std::getline(file_input_, address))
    {
        try
        {
            unsigned int address = std::stoul(address);
            std::string hex_address;

            INT_TO_HEX(address, hex_address)

            Tins::DNS query;
            query.id(1338);
            query.recursion_desired(1);

            query.add_query(
                Tins::DNS::query(
                    hex_address + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu"
                    1,
                    1
                )
            );

            std::vector<uint8_t> binary_query = query.serialize();
            std::vector<uint8_t> full_packet(binary_query.size()+8);

            CRAFT_QUERY_PACKET(full_packet, binary_query)

            struct in_addr ip_address;
            ip_address.s_addr = address;

            boost::asio::ip::raw::endpoint end_point(
                boost::asio::ip::address::from_string(inet_ntoa(ip_address)),
                UDPSender::remote_port_num
            );

            socket_.async_send_to(
                boost::asio::buffer(full_packet),
                end_point,
                boost::bind(
                    &UDPSender::handle_send,
                    this,
                    boost::asio::placeholder::error,
                    boost::asio::placeholders::bytes_transferred
                )
            );
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    }
}

void UDPSender::handle_send(const boost::system::error_code& error_code, std::size_t)
{
}

