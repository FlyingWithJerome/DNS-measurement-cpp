#include "packet_factory.hpp"

constexpr size_t QueryFactory::udp_header_size;
constexpr size_t QueryFactory::tcp_dns_size_shift;

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
            Tins::DNS::QueryType(Tins::DNS::QueryType::A),
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
            Tins::DNS::QueryType(Tins::DNS::QueryType::A),
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
    Tins::DNS query;
    query.id(packet_config.id);
    query.recursion_desired(1);
    query.add_query(
        Tins::DNS::query(
            packet_config.q_name,
            Tins::DNS::QueryType(Tins::DNS::QueryType::A),
            Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET)
        )
    );
    std::vector<uint8_t> binary_query = query.serialize();
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


ResponseFactory::ResponseFactory()
: answer_starts(
    std::string(ANSWER_STARTS)
)
{
    for (int num_of_resource=0; num_of_resource < NUMBER_OF_LONG_ENTRIES; num_of_resource++)
    {
        tcp_answers.push_back(answer_starts + std::to_string(num_of_resource));
    }

    for (int num_of_resource=0; num_of_resource < 26; num_of_resource++)
    {
        ns_answers.push_back(
            std::string(ns_server_names[num_of_resource]) + "-yumi.ipl.eecs.case.edu"
        );
    }
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
        for(const Tins::DNS::query& query : question.queries())
        {
            response.add_query(query);
        }

        Tins::DNS::QueryClass query_class = response.queries()[0].query_class();
        Tins::DNS::QueryType  query_type  = response.queries()[0].query_type();

        if (not query_property.will_truncate or query_property.jumbo_type == NameUtilities::JumboType::jumbo_one_answer)
        {
            switch (query_type)
            {
                case Tins::DNS::QueryType::A:
                    APPEND_ANSWER(tcp, number_of_answer_entries)
                    break;
                
                case Tins::DNS::QueryType::NS:
                    APPEND_ANSWER(ns, number_of_answer_entries)
                    break;

                case Tins::DNS::QueryType::MX:
                    APPEND_ANSWER(ns, number_of_answer_entries)
                    break;

                case Tins::DNS::QueryType::TXT:
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
    
    packet_to_be_filled = response.serialize();

    if (query_property.expect_answer_count != UINT8_MAX)
    {
        packet_to_be_filled[6] = (uint8_t)(query_property.expect_answer_count >> 8);
        packet_to_be_filled[7] = (uint8_t)(query_property.expect_answer_count & 0xff);
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
        for(const Tins::DNS::query& query : question.queries())
        {
            response.add_query(query);
        }

        Tins::DNS::QueryClass query_class = response.queries()[0].query_class();
        Tins::DNS::QueryType  query_type  = response.queries()[0].query_type();

        switch (query_type)
        {
            case Tins::DNS::QueryType::A:
                for(const std::string& answer : tcp_answers)
                {
                    response.add_answer(
                        Tins::DNS::resource(
                            query_property.name,
                            answer,
                            query_type,
                            query_class,
                            DNS_RESOURCE_TTL
                        )
                    );
                }
                break;
            
            case Tins::DNS::QueryType::NS:
                for(const std::string& answer : ns_answers)
                {
                    response.add_answer(
                        Tins::DNS::resource(
                            query_property.name,
                            answer,
                            query_type,
                            query_class,
                            DNS_RESOURCE_TTL
                        )
                    );
                }
                break;

            case Tins::DNS::QueryType::MX:
                for(const std::string& answer : ns_answers)
                {
                    response.add_answer(
                        Tins::DNS::resource(
                            query_property.name,
                            answer,
                            query_type,
                            query_class,
                            DNS_RESOURCE_TTL
                        )
                    );
                }
                break;

            case Tins::DNS::QueryType::TXT:
                break;

            default:
                break;
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
