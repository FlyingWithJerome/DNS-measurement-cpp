#include "udp_scanner_listener.hpp"

constexpr uint16_t UDPListener::local_port_num;
constexpr uint16_t UDPListener::remote_port_num;

constexpr uint32_t UDPListener::rcv_buf_size;

UDPListener::UDPListener(
    boost::asio::io_service& io_service,
    std::shared_ptr<boost::interprocess::message_queue>& message_queue_,
    std::atomic<bool>& ddos_hold_on
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
, number_of_recv_responses_(
    LRU_SIZE
)
{
    boost::asio::socket_base::receive_buffer_size option(rcv_buf_size);
    main_socket_.set_option(option);

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Listener] UDP socket buffer size " << buf_size.value() << "\n";

    main_socket_.non_blocking(true);

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
        // std::cout << "[UDP Listener] err msg " << error_code.message() << std::endl;
    }
    else
    {
        boost::asio::ip::udp::endpoint sender_info;
        boost::system::error_code ec;
        std::size_t available_packet_size = main_socket_.available(ec);

        try
        {
            std::vector<uint8_t> new_arrival(available_packet_size);

            available_packet_size = main_socket_.receive_from(
                boost::asio::buffer(new_arrival),
                sender_info
            );

            new_arrival.resize(available_packet_size);

            if (new_arrival.size() >= 20)
            {
                handle_receive(new_arrival, sender_info);
            }
            else
            {
                start_receive();
            }
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

    bool is_valid_dns_packet;
    std::pair<std::string, int> name_type_pair;

    try
    {
        incoming_response = Tins::DNS(incoming_packet.data(), incoming_packet.size());

        name_type_pair = extract_name_type_pair(incoming_response);
        number_of_answers = incoming_response.answers().size();

        if (name_type_pair.second != INVALID_QUERY_TYPE)
        {
            int number_of_entries = query_lru(number_of_recv_responses_, name_type_pair.first);
            if (number_of_entries != ENTRY_NOT_EXIST)
            {
                UDP_SCANNER_REPEAT_RESPONSE_LOG(sender, name_type_pair.first)

                goto End;
            }
        }
    }
    catch(...)
    {
        std::cout << "[UDP Listener] received a malformed packet\n";
        goto End;
    }  

    std::cout 
    << "[UDP Listener] Address: " 
    << sender.address().to_string()
    << " rcode: "
    << (int)incoming_response.rcode()
    << " answer count: "
    << number_of_answers
    << "\n";    

    if (incoming_response.rcode() == 0)
    { // is a legal response

        if (name_type_pair.second != INVALID_QUERY_TYPE)
        {
            const std::string& question_name = name_type_pair.first;
            const int& type_of_query_ = name_type_pair.second;
            const NameUtilities::QueryProperty query_property(question_name);
            
            if(query_property.jumbo_type == NameUtilities::JumboType::no_jumbo) // not a jumbo query, will start a jumbo query
            {                
                const std::string question_name_jumbo_no_answer     = std::string("jumbo1-") + query_property.name;
                const std::string question_name_jumbo_one_answer    = std::string("jumbo2-") + query_property.name;
                // const std::string question_name_ac1an0 = std::string("ac1an0-") + question_name_jumbo_one_answer;
                const std::string question_name_jumbo_broken_answer = std::string("jumbo3-") + query_property.name;
                const std::string question_name_for_tcp             = std::string("t-") + query_property.name;

                SEND_OUT_PACKET(jumbo_n_a,   question_name_jumbo_no_answer,   QueryType::A, sender)
                SEND_OUT_PACKET(jumbo_o_a,   question_name_jumbo_one_answer,  QueryType::A, sender)
                // SEND_OUT_PACKET(ac1an0_a,  question_name_ac1an0, QueryType::A, sender)
                SEND_OUT_PACKET(jumbo_b_a, question_name_jumbo_broken_answer, QueryType::A, sender)

                SEND_OUT_PACKET(jumbo_n_mx,   question_name_jumbo_no_answer,   QueryType::MX, sender)
                SEND_OUT_PACKET(jumbo_o_mx,   question_name_jumbo_one_answer,  QueryType::MX, sender)
                // SEND_OUT_PACKET(ac1an0_mx,  question_name_ac1an0, QueryType::MX, sender)
                SEND_OUT_PACKET(jumbo_b_mx, question_name_jumbo_broken_answer, QueryType::MX, sender)

                SEND_OUT_PACKET(jumbo_n_txt,   question_name_jumbo_no_answer,   QueryType::TXT, sender)
                SEND_OUT_PACKET(jumbo_o_txt,   question_name_jumbo_one_answer,  QueryType::TXT, sender)
                // SEND_OUT_PACKET(ac1an0_txt,  question_name_ac1an0, QueryType::TXT, sender)
                SEND_OUT_PACKET(jumbo_b_txt, question_name_jumbo_broken_answer, QueryType::TXT, sender)

                SEND_TO_TCP_SCANNER(question_name_for_tcp, QueryType::A)
                UDP_SCANNER_NORMAL_LOG(sender, query_property.question_id, "ok")

            }// not a jumbo query, will start a jumbo query END
            else if (incoming_response.truncated()) // is a jumbo query and is truncated (as our expectation)
            {
                SEND_TO_TCP_SCANNER(question_name, type_of_query_)
                UDP_SCANNER_TRUNCATE_LOG(sender, query_property.question_id, number_of_answers, type_of_query_, "tc_ok")
            }
            else // is a jumbo query and is not truncated but has a some answers
            {
                UDP_SCANNER_TRUNCATE_LOG(sender, query_property.question_id, number_of_answers, type_of_query_, "tc_fail")
            }
        }
        else // query type is invalid (no queries or answers included)
        {
            UDP_SCANNER_BAD_RESPONSE_LOG(
                sender, 
                INVALID_QUESTION_ID, 
                incoming_response.rcode(), 
                INVALID_QUERY_TYPE,
                INVALID_JUMBO_TYPE, 
                0, 
                "no_(qr&an)records_included"
            )
        }
    }
    else // rcode is not NOERROR
    {
        UDP_SCANNER_BAD_RESPONSE_LOG(
            sender, 
            INVALID_QUESTION_ID, 
            incoming_response.rcode(), 
            INVALID_QUERY_TYPE,
            INVALID_JUMBO_TYPE, 
            INVALID_NUM_OF_ANSWERS, 
            "non_zero_rcode"
        )
    }

    End:
    start_receive();
}

void UDPListener::handle_send(const boost::system::error_code& error_code, std::size_t)
{
    // if(error_code)
        // std::cout << error_code.message() << std::endl;
}

UDPListener::~UDPListener()
{
    boost::interprocess::message_queue::remove("pipe_to_tcp");
}
