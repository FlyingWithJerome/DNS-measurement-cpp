#include "udp_scanner_listener.hpp"

constexpr uint16_t UDPListener::local_port_num;
constexpr uint16_t UDPListener::remote_port_num;

constexpr char UDPListener::udp_normal_log_[];
constexpr char UDPListener::udp_truncate_log_[];
constexpr char UDPListener::udp_bad_response_log_[];

constexpr uint32_t UDPListener::rcv_buf_size;

UDPListener::UDPListener(
    boost::asio::io_service& io_service,
    std::shared_ptr<boost::interprocess::message_queue>& message_queue_
)
: main_socket_(
    io_service,
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("0.0.0.0"), 
        local_port_num
    )
)
, pipe_to_tcp_(
    message_queue_
)
{
    boost::asio::socket_base::receive_buffer_size option(rcv_buf_size);
    main_socket_.set_option(option);

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Listener] UDP socket buffer size " << buf_size.value() << "\n";

    main_socket_.non_blocking(true);

    init_new_log_file(udp_normal_log_);
    init_new_log_file(udp_truncate_log_);
    init_new_log_file(udp_bad_response_log_);

    this->start_receive();
}


void UDPListener::start_receive()
{
    main_socket_.async_receive_from(
        boost::asio::null_buffers(),
        remote_endpoint_,
        boost::bind(
            &UDPListener::reactor_read, 
            this,
            boost::asio::placeholders::error
        )
    );
}

boost::asio::ip::udp::socket& UDPListener::get_socket()
{
    return main_socket_;
}

void UDPListener::reactor_read(const boost::system::error_code& error_code)
{
    if(error_code)
    {
        std::cout << "[UDP Listener] err msg " << error_code.message() << std::endl;
        start_receive();
    }
    else
    {
        boost::asio::ip::udp::endpoint sender_info;
        
        std::size_t available_packet_size = std::min((int)(main_socket_.available()), 1000);

        try
        {
            std::vector<uint8_t> new_arrival(available_packet_size);

            available_packet_size = main_socket_.receive_from(
                boost::asio::buffer(new_arrival),
                sender_info
            );

            new_arrival.resize(available_packet_size);

            handle_receive(new_arrival, sender_info);
        }
        catch(...)
        {
            start_receive();
        }
    }
}

void UDPListener::handle_receive(
    const std::vector<uint8_t>& incoming_packet, 
    const boost::asio::ip::udp::endpoint& sender
)
{
    Tins::DNS incoming_response;
    int number_of_answers;

    try
    {
        incoming_response = Tins::DNS(incoming_packet.data(), incoming_packet.size());
    }
    catch(...)
    {
        std::cout << "[UDP Listener] received a malformed packet\n";
        goto End;
    }

    number_of_answers = incoming_response.answers_count();

    std::cout 
    << "[UDP Listener] Address: " 
    << sender.address().to_string()
    // << " Name: "
    // << incoming_response.queries()[0].dname()
    << " rcode: "
    << (int)incoming_response.rcode()
    << " answer count: "
    << number_of_answers
    << "\n";    

    if (incoming_response.rcode() == 0)
    { // is a legal response

        if (number_of_answers > 0)
        {
            std::string question_name = incoming_response.answers()[0].dname();

            NameUtilities::QueryProperty query_property(question_name);
            
            if(query_property.jumbo_type == NameUtilities::JumboType::no_jumbo) // not a jumbo query, will start a jumbo query
            {
                std::cout << "[UDP Listener] Name: " << question_name << std::endl;
                
                std::vector<uint8_t> full_packet_jumbo;
                std::vector<uint8_t> full_packet_ac1an0;
                const QueryType type_of_query_ = incoming_response.queries()[0].query_type();
                
                const std::string question_name_jumbo  = std::string("jumbo1-") + query_property.name;
                const std::string question_name_ac1an0 = std::string("ac1an0-") + question_name_jumbo;
                const std::string question_for_tcp     = std::string("t-") + query_property.name;

                SEND_OUT_PACKET(jumbo,  full_packet_jumbo,  question_name_jumbo,  type_of_query_, sender)
                SEND_OUT_PACKET(ac1an0, full_packet_ac1an0, question_name_ac1an0, type_of_query_, sender)

                SEND_TO_TCP_SCANNER(question_for_tcp)
                UDP_SCANNER_NORMAL_LOG(udp_normal_log_, sender, query_property.question_id, "ok")

            }// not a jumbo query, will start a jumbo query END
            else if (incoming_response.truncated()) // is a jumbo query and is truncated (as our expectation)
            {
                // send to tcp scanner
                SEND_TO_TCP_SCANNER(question_name)
                UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, query_property.question_id, number_of_answers, "tc_ok")
            }
            else // is a jumbo query and is not truncated but has a some answers
            {
                UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, query_property.question_id, number_of_answers, "tc_fail")
            }
        }
        else // answer count is 0
        {
            std::string question_name;
            NameUtilities::QueryProperty query_property;

            if (incoming_response.questions_count() > 0)
            {
                // extract the query name to get its query property
                question_name = incoming_response.queries()[0].dname();
                query_property = NameUtilities::QueryProperty(question_name);

                if (incoming_response.truncated()) // 0 answer but is truncated (as our expectation)
                {
                    // send to tcp scanner
                    SEND_TO_TCP_SCANNER(question_name)
                    UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, query_property.question_id, 0, "tc_ok")
                }
                else // 0 answer and is not truncated
                {
                    if (query_property.jumbo_type == NameUtilities::JumboType::no_jumbo) // 0 answer and is not truncated and does not need to be truncated
                    { // 0 answer and is not truncated and we are NOT expecting truncation
                        UDP_SCANNER_BAD_RESPONSE_LOG(
                            udp_bad_response_log_, 
                            sender, 
                            query_property.question_id, 
                            incoming_response.rcode(),
                            (int)query_property.jumbo_type,
                            incoming_response.answers_count(),
                            "--"
                        )
                    }
                    else // 0 answer and is not truncated and we are expecting truncation
                    {
                        UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, query_property.question_id, 0, "tc_fail")
                    }   
                }
            }
            else // 0 answer and is not truncated and has no questions included (no way to tell its query property)
            {
                UDP_SCANNER_BAD_RESPONSE_LOG(udp_bad_response_log_, sender, 0, incoming_response.rcode(), -1, 0, "no_records_included")
            } 
        } // answer count is 0 END
    }
    else // rcode is not NOERROR
    {
        UDP_SCANNER_BAD_RESPONSE_LOG(udp_bad_response_log_, sender, 0, incoming_response.rcode(), -1, 0, "non_zero_rcode")
    }

    End:
    start_receive();
}

void UDPListener::handle_send(std::vector<uint8_t>&, const boost::system::error_code& error_code, std::size_t)
{
    if(error_code)
        std::cout << error_code.message() << std::endl;
}

UDPListener::~UDPListener()
{
    boost::interprocess::message_queue::remove("pipe_to_tcp");
}
