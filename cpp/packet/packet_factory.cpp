#include "packet_factory.hpp"

constexpr size_t QueryFactory::udp_header_size;
constexpr size_t QueryFactory::tcp_dns_size_shift;

QueryType string_to_query_type(const std::string& str) noexcept
{
    if (str == "A")
        return QueryType::A;

    else if (str == "MX")
        return QueryType::MX;

    else if (str == "NS")
        return QueryType::NS;

    else if (str == "TXT")
        return QueryType::TXT;

    return QueryType::A;
}

void QueryFactory::make_packet(
    const PacketTypes& packet_type,
    const packet_configuration& packet_config,
    std::vector<uint8_t>& packet_to_be_filled
)
{
    switch (packet_type)
    {
        case UDP_QUERY:
            return make_udp_query(packet_config, packet_to_be_filled);
        
        case TCP_QUERY:
            return make_tcp_query(packet_config, packet_to_be_filled);

        case RAW_QUERY:
            return make_raw_query(packet_config, packet_to_be_filled);

        default:
            break;
    }
}


void QueryFactory::make_udp_query(
    const packet_configuration& packet_config,
    std::vector<uint8_t>& packet_to_be_filled
)
{
    Tins::DNS query;
    query.id(packet_config.id);
    query.recursion_desired(1);
    query.add_query(
        Tins::DNS::query(
            packet_config.q_name,
            packet_config.query_type,
            Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET)
        )
    );
    packet_to_be_filled = query.serialize();
}

void QueryFactory::make_tcp_query(
    const packet_configuration& packet_config,
    std::vector<uint8_t>& packet_to_be_filled
)
{
    Tins::DNS query;
    query.id(packet_config.id);
    query.recursion_desired(1);
    query.add_query(
        Tins::DNS::query(
            packet_config.q_name,
            packet_config.query_type,
            Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET)
        )
    );
    std::vector<uint8_t> binary_query = query.serialize();
    packet_to_be_filled.reserve(binary_query.size() + QueryFactory::tcp_dns_size_shift);

    uint8_t header[QueryFactory::tcp_dns_size_shift] = {
        (uint8_t)((binary_query.size() >> 8)), (uint8_t)((binary_query.size() & 0xff))
    };

    packet_to_be_filled.assign(header, header + QueryFactory::tcp_dns_size_shift);
    packet_to_be_filled.insert(
        packet_to_be_filled.begin() + QueryFactory::tcp_dns_size_shift, 
        binary_query.begin(), 
        binary_query.end()
    );
}

void QueryFactory::make_raw_query(
    const packet_configuration& packet_config,
    std::vector<uint8_t>& packet_to_be_filled
)
{
    std::vector<uint8_t> binary_query;
    make_udp_query(packet_config, binary_query);
    const size_t full_packet_size     = binary_query.size() + QueryFactory::udp_header_size;
    packet_to_be_filled.reserve(full_packet_size);

    const uint8_t header[QueryFactory::udp_header_size] = {
        0xb, 
        0xb7, 
        0, 
        0x35, 
        (uint8_t)(full_packet_size >> 8), 
        (uint8_t)(full_packet_size & 0xff), 
        0, 
        0
    };
    packet_to_be_filled.assign(header, header+QueryFactory::udp_header_size);
    packet_to_be_filled.insert(
        packet_to_be_filled.begin() + QueryFactory::udp_header_size, 
        binary_query.begin(), 
        binary_query.end()
    );
}

void udp_a_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL
            )
        ); 
    }
}

void udp_ns_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL
            )
        ); 
    }
}

void udp_mx_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    dns_packet.truncated(1);
}

void udp_txt_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL
            )
        ); 
    }
}

void tcp_a_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL
            )
        ); 
    }
}

void tcp_ns_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL
            )
        ); 
    }
}

void tcp_mx_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL,
                10
            )
        ); 
    }
}

void tcp_txt_handler(
    Tins::DNS& dns_packet, 
    const std::vector<std::string>& answer_entries,
    const Tins::DNS::QueryClass& query_class,
    const Tins::DNS::QueryType& query_type,
    const std::string& query_name
)
{
    for(const std::string& answer : answer_entries)
    {
        dns_packet.add_answer(
            Tins::DNS::resource(
                query_name,
                answer,
                query_type,
                query_class,
                DNS_RESOURCE_TTL,
                10
            )
        ); 
    }
}

ResponseFactory::ResponseFactory()
: answer_starts(
    std::string(ANSWER_STARTS)
)
{
    for (int num_of_resource=0; num_of_resource < NUMBER_OF_LONG_ENTRIES; num_of_resource++)
    {
        a_answers.push_back(answer_starts + std::to_string(num_of_resource));
    }

    for (int num_of_resource=0; num_of_resource < 26; num_of_resource++)
    {
        ns_answers.push_back(form_unsuppressable_hostname(num_of_resource));
    }

    mx_answers = ns_answers;

    std::ifstream file("cpp/server/some_text.txt");

    txt_answer = ResponseFactory::form_txt_string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );

    std::cout << "[Packet Factory] txt answer size: " << txt_answer.size() << std::endl;

    short_txt_answer = std::string(short_msg);
    NameUtilities::make_dns_style_string(short_txt_answer);
}

std::string ResponseFactory::form_unsuppressable_hostname(const int& rotate_index) const
{
    std::string result;
    std::vector<const char*> after_rotate(NUMBER_OF_CALLSIGNS);
    std::rotate_copy(
        ns_server_names, 
        ns_server_names+rotate_index, 
        ns_server_names+NUMBER_OF_CALLSIGNS,
        after_rotate.begin()
    );

    for (int index = 0; index < after_rotate.size() and result.size() <= MAX_HOSTNAME_WITHOUT_DOT_SIZE; index++)
    {
        result += after_rotate[index];
    }

    for (int index = after_rotate.size()-1; index >= 0 and result.size() <= MAX_HOSTNAME_WITHOUT_DOT_SIZE; index--)
    {
        result += after_rotate[index];
    }

    result = result.substr(0, MAX_HOSTNAME_WITHOUT_DOT_SIZE);

    for (int index = MAX_LABEL_SIZE; index < result.size(); index+=MAX_LABEL_SIZE)
    {
        result.insert(index, 1, '.');
    }

    return result;
}

void ResponseFactory::make_packet(
    const PacketTypes&                  packet_type,
    const Tins::DNS&                    question,
    const NameUtilities::QueryProperty& query_property,
    std::vector<uint8_t>&               packet_to_be_filled
) const
{
    switch(packet_type)
    {
        case UDP_RESPONSE:
            return make_udp_response(question, query_property, packet_to_be_filled);

        case TCP_RESPONSE:
            return make_tcp_response(question, query_property, packet_to_be_filled);

        default:
            break;
    }
}

void ResponseFactory::make_udp_response(
    const Tins::DNS&                    question,
    const NameUtilities::QueryProperty& query_property,
    std::vector<uint8_t>&               packet_to_be_filled
) const
{
    Tins::DNS response;
    response.id(question.id());
    response.truncated(query_property.will_truncate);
    response.type(Tins::DNS::QRType::RESPONSE);
    response.opcode(DNS_OPCODE_REPLY);

    int number_of_answer_entries = (query_property.expect_number_of_answers == UINT8_MAX) ? 
    1 : query_property.expect_number_of_answers;

    if (query_property.is_authoritative)
    {
        response.authoritative_answer(1);

        for(const Tins::DNS::query& query : question.queries())
        {
            response.add_query(query);
        }

        if (response.queries().size() > 0)
        {
            std::string spf_response(SPF_RESPONSE);
            Tins::DNS::QueryClass query_class = response.queries()[0].query_class();
            Tins::DNS::QueryType  query_type  = response.queries()[0].query_type();

            if (not query_property.will_truncate or query_property.jumbo_type == NameUtilities::JumboType::jumbo_one_answer)
            {
                auto handler = query_handler.find(
                    std::make_pair(ProtocolTypes::PROTO_UDP, query_type)
                );
                switch (query_type)
                {
                    case Tins::DNS::QueryType::A:
                        handler->second
                        (
                            response,
                            a_answers,
                            query_class,
                            query_type,
                            query_property.name
                        );
                        break;
                    
                    case Tins::DNS::QueryType::NS:
                        handler->second
                        (
                            response,
                            ns_answers,
                            query_class,
                            query_type,
                            query_property.name
                        );
                        break;

                    case Tins::DNS::QueryType::MX:
                        handler->second
                        (
                            response,
                            mx_answers,
                            query_class,
                            query_type,
                            query_property.name
                        );
                        break;

                    case Tins::DNS::QueryType::TXT:
                        handler->second
                        (
                            response,
                            std::vector<std::string>{short_txt_answer},
                            query_class,
                            query_type,
                            query_property.name
                        );
                        break;

                    case SPF_TYPE:
                        NameUtilities::make_dns_style_string(spf_response);
                        response.add_answer(
                            Tins::DNS::resource(
                                query_property.name,
                                spf_response.c_str(),
                                query_type,
                                query_class,
                                DNS_RESOURCE_TTL
                            )
                        );
                        break;

                    default:
                        break;
                }
            } // if it does not need to be truncated or it does need one answer
        } // if (response.queries().size() > 0)
    }
    else if ( query_property.jumbo_type == NameUtilities::JumboType::jumbo_no_answer )
    {
        response.rcode(DNS_RCODE_NOERROR);
    }
    else
    {
        response.rcode(DNS_RCODE_REFUSED);
    }
    
    packet_to_be_filled = response.serialize();

    if (query_property.expect_answer_count != UINT8_MAX)
    {
        packet_to_be_filled[6] = (uint8_t)(query_property.expect_answer_count >> 8);
        packet_to_be_filled[7] = (uint8_t)(query_property.expect_answer_count & 0xff);
    }

    if (query_property.jumbo_type == NameUtilities::JumboType::jumbo_broken_answer)
    {
        packet_to_be_filled.resize(packet_to_be_filled.size() - 5);
    }
}

void ResponseFactory::make_tcp_response(
    const Tins::DNS&                    question,
    const NameUtilities::QueryProperty& query_property,
    std::vector<uint8_t>&               packet_to_be_filled
) const
{
    Tins::DNS response;
    response.id(question.id());
    response.type(Tins::DNS::QRType::RESPONSE);
    response.opcode(DNS_OPCODE_REPLY);

    if (query_property.is_authoritative)
    {
        response.authoritative_answer(1);
        
        for(const Tins::DNS::query& query : question.queries())
        {
            response.add_query(query);
        }

        if (response.queries().size() > 0)
        {
            const Tins::DNS::QueryClass query_class = response.queries()[0].query_class();
            const Tins::DNS::QueryType  query_type  = response.queries()[0].query_type();

            auto handler = query_handler.find(
                std::make_pair(ProtocolTypes::PROTO_TCP, query_type)
            );

            switch (query_type)
            {
                case Tins::DNS::QueryType::A:
                    handler->second
                    (
                        response,
                        a_answers,
                        query_class,
                        query_type,
                        query_property.name
                    );
                    break;
                
                case Tins::DNS::QueryType::NS:
                    handler->second
                    (
                        response,
                        ns_answers,
                        query_class,
                        query_type,
                        query_property.name
                    );
                    break;

                case Tins::DNS::QueryType::MX:
                    handler->second
                    (
                        response,
                        std::vector<std::string>{ "cluster32.case.edu" },
                        query_class,
                        query_type,
                        query_property.name
                    );
                    break;

                case Tins::DNS::QueryType::TXT:
                    handler->second
                    (
                        response,
                        std::vector<std::string>{ txt_answer },
                        query_class,
                        query_type,
                        query_property.name
                    );
                    break;

                default:
                    break;
            }
        }
    }
    else
    {
        response.rcode(DNS_RCODE_REFUSED);
    }

    uint16_t size_of_packet = (uint16_t)response.size();
    uint8_t res_size[] = {(uint8_t)(size_of_packet >> 8), (uint8_t)(size_of_packet & 0xFF)};

    packet_to_be_filled = response.serialize();
    packet_to_be_filled.insert(packet_to_be_filled.begin(), res_size, res_size+2);
}
