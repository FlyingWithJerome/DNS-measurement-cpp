#include "udp_scanner_listener.hpp"

constexpr uint16_t UDPListener::local_port_num;
constexpr uint16_t UDPListener::remote_port_num;

constexpr char UDPListener::udp_normal_log_[];
constexpr char UDPListener::udp_truncate_log_[];
constexpr char UDPListener::udp_bad_response_log_[];

constexpr uint32_t UDPListener::rcv_buf_size;

UDPListener::UDPListener(boost::asio::io_service& io_service)
: main_socket_(
    io_service,
    boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("0.0.0.0"), 
        local_port_num
    )
)
{
    boost::interprocess::message_queue::remove("pipe_to_tcp");
    pipe_to_tcp_.reset(
        new boost::interprocess::message_queue(
            boost::interprocess::create_only,
            "pipe_to_tcp",
            1000,
            sizeof(message_pack)
        )
    );

    boost::asio::socket_base::receive_buffer_size option(rcv_buf_size);
    main_socket_.set_option(option);

    boost::asio::socket_base::receive_buffer_size buf_size;
    main_socket_.get_option(buf_size);

    std::cout << "[UDP Listener] UDP socket buffer size " << buf_size.value() << std::endl;

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

        std::vector<uint8_t> new_arrival(available_packet_size);

        available_packet_size = main_socket_.receive_from(
            boost::asio::buffer(new_arrival),
            sender_info
        );

        new_arrival.resize(available_packet_size);

        handle_receive(new_arrival, sender_info);
    }
}

void UDPListener::handle_receive(
    const std::vector<uint8_t>& incoming_packet, 
    const boost::asio::ip::udp::endpoint& sender
)
{
    Tins::DNS incoming_response;
    try
    {
        incoming_response = Tins::DNS(incoming_packet.data(), incoming_packet.size());
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
    << incoming_response.answers_count()
    << "\n";

    if (incoming_response.rcode() == 0 and incoming_response.answers_count() > 0)
    { // is a legal response
        std::string question_name = incoming_response.answers()[0].dname();
        std::transform(question_name.begin(), question_name.end(), question_name.begin(), ::tolower);

        uint32_t question_id = NameTrick::get_question_id(question_name);

        NameTrick::JumboType jumbo_type_ = NameTrick::get_jumbo_type(question_name);
        
        if(jumbo_type_ == NameTrick::JumboType::no_jumbo) // not a jumbo query, will start a jumbo query
        {
            if (incoming_response.answers()[0].data() != "192.168.0.0")
            {
                // write down the answer error
                UDP_SCANNER_BAD_RESPONSE_LOG(
                    udp_bad_response_log_,
                    sender,
                    question_id,
                    0,
                    (int)jumbo_type_,
                    incoming_response.answers_count(),
                    "answer error"
                )
                goto End;
            }
            std::vector<uint8_t> packet;
            std::string          hex_address;

            INT_TO_HEX(question_id, hex_address)

            std::vector<uint8_t> full_packet;
            std::string question = std::string("jumbo1-") 
            + hex_address 
            + "-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";

            CRAFT_FULL_QUERY_UDP(question, full_packet)

            main_socket_.async_send_to(
                boost::asio::buffer(full_packet),
                sender,
                boost::bind(
                    &UDPListener::handle_send,
                    this,
                    full_packet,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                )
            );
            
            UDP_SCANNER_NORMAL_LOG(udp_normal_log_, sender, question_id, "ok")
        }
        else
        {
            if (incoming_response.truncated()) // is a jumbo code and is truncated (as our expectation)
            {
                // send to tcp scanner
                message_pack outgoing;

                strcpy(outgoing.ip_address, sender.address().to_string().c_str());
                strcpy(outgoing.question,   question_name.c_str());

                pipe_to_tcp_->send(&outgoing, sizeof(outgoing), 1);
                UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, question_id, "tc_ok")
            }
            else // is a jumbo query but is not truncated
            {
                UDP_SCANNER_TRUNCATE_LOG(udp_truncate_log_, sender, question_id, "tc_fail")
            }
        }
    }
    else // is not even a legal udp packet
    {
        std::string question_name;
        if (incoming_response.answers_count() > 0)
            question_name = incoming_response.answers()[0].dname();

        else if (incoming_response.questions_count() > 0)
            question_name = incoming_response.queries()[0].dname();

        else
        {
            UDP_SCANNER_BAD_RESPONSE_LOG(
                udp_bad_response_log_, 
                sender, 
                0, 
                incoming_response.rcode(),
                -1,
                incoming_response.answers_count(),
                "no_records_included"
            )
            goto End;
        }

        std::transform(question_name.begin(), question_name.end(), question_name.begin(), ::tolower);
        uint32_t question_id            = NameTrick::get_question_id(question_name);
        NameTrick::JumboType jumbo_type = NameTrick::get_jumbo_type(question_name);

        UDP_SCANNER_BAD_RESPONSE_LOG(
            udp_bad_response_log_, 
            sender, 
            question_id, 
            incoming_response.rcode(),
            (int)jumbo_type,
            incoming_response.answers_count(),
            "--"
        )
    } // is not even a legal udp packet END----

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
