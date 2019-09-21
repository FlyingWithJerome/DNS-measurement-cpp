#ifndef PACKET_FACTORY_
#define PACKET_FACTORY_

#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <functional>
#include <tins/dns.h>
#include <ctype.h>
#include <string>
#include <vector>

#include <tins/dns.h>

#include "name_util.hpp"
#include "../constants.hpp"

#define MAX_LABEL_SIZE                63
#define MAX_HOSTNAME_WITHOUT_DOT_SIZE 248
#define NUMBER_OF_CALLSIGNS           26

#define SPF_TYPE 99
#define SPF_RESPONSE "v=spf1 ip4:192.5.110.81 ip4:129.22.150.52 ~all"

#define INT_TO_HEX(input_int, output_str) char out[20]; \
sprintf(out, "0x%x", input_int); \
output_str = out;

#define RESPONSE_MAKER_COMMON(question, property)  Tins::DNS response; \
response.id(question.id()); \
response.type(Tins::DNS::QRType::RESPONSE); \
response.opcode(DNS_OPCODE_REPLY); \
if (property.is_authoritative) { \
for(const Tins::DNS::query& res_ : question.queries()) { \
response.add_query(res_);}

enum PacketTypes {
    RAW_QUERY,
    UDP_QUERY,
    UDP_RESPONSE,
    TCP_QUERY,
    TCP_RESPONSE,
};

enum ProtocolTypes {
    PROTO_TCP,
    PROTO_UDP
};

using QueryType = Tins::DNS::QueryType;

typedef struct{
    uint32_t             id;
    Tins::DNS::QueryType query_type;
    int                  expect_answer_count;
    int                  actual_num_ans;
    std::string          q_name;
    
} packet_configuration;

typedef std::function<
    void(
        Tins::DNS&, 
        const std::vector<std::string>&,
        const Tins::DNS::QueryClass&,
        const Tins::DNS::QueryType&,
        const std::string&
    )
> handler;

void udp_a_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void udp_ns_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void udp_mx_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void udp_txt_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void tcp_a_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void tcp_ns_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void tcp_mx_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

void tcp_txt_handler(
    Tins::DNS&, 
    const std::vector<std::string>&,
    const Tins::DNS::QueryClass&,
    const Tins::DNS::QueryType&,
    const std::string&
);

const std::map<std::pair<ProtocolTypes, QueryType>, handler> query_handler = {
    {std::make_pair(ProtocolTypes::PROTO_UDP, QueryType::A),   udp_a_handler},
    {std::make_pair(ProtocolTypes::PROTO_TCP, QueryType::A),   tcp_a_handler},
    {std::make_pair(ProtocolTypes::PROTO_UDP, QueryType::NS),  udp_ns_handler},
    {std::make_pair(ProtocolTypes::PROTO_TCP, QueryType::NS),  tcp_ns_handler},
    {std::make_pair(ProtocolTypes::PROTO_UDP, QueryType::MX),  udp_mx_handler},
    {std::make_pair(ProtocolTypes::PROTO_TCP, QueryType::MX),  tcp_mx_handler},
    {std::make_pair(ProtocolTypes::PROTO_UDP, QueryType::TXT), udp_txt_handler},
    {std::make_pair(ProtocolTypes::PROTO_TCP, QueryType::TXT), tcp_txt_handler}
};

template <typename input_iter>
static std::string form_txt_string(
    input_iter start,
    input_iter end
)
{
    const int max_size = 255;

    std::string result;
    std::string txt_segment;

    while ( start != end )
    {
        while ( txt_segment.size() < max_size and start != end )
        {   
            txt_segment += (*start++);
        }

        txt_segment.insert(0, 1, (char)txt_segment.size());
        result.append(txt_segment);
        txt_segment.clear();
    }

    std::replace(result.begin(), result.end(), '\n', ' ');
    return result;
};

static constexpr char short_msg[] = 
"an Internet measurement on DNS in IPv4 space, "
"carried out by Jerome Mao and Professor Michael Rabinovich "
"from Case Western Reserve University (jxm959*at*case.edu).";

static constexpr const char* ns_server_names[NUMBER_OF_CALLSIGNS] = {
    "Alfa",
    "Bravo",
    "Charlie",
    "Delta",
    "Echo",
    "Foxtrot",
    "Golf",
    "Hotel",
    "India",
    "Juliett",
    "Kilo",
    "Lima",
    "Mike",
    "November",
    "Oscar",
    "Papa",
    "Quebec",
    "Romeo",
    "Sierra",
    "Tango",
    "Uniform",
    "Victor",
    "Whiskey",
    "Xray",
    "Yankee",
    "Zulu"
};

QueryType string_to_query_type(const std::string&) noexcept;

class QueryFactory
{
    public:
        void make_packet(
            const PacketTypes&, 
            const packet_configuration&, 
            std::vector<uint8_t>&
        );

        static constexpr size_t udp_header_size    = 8;
        static constexpr size_t tcp_dns_size_shift = 2;

    private:
        void make_udp_query(const packet_configuration&, std::vector<uint8_t>&);
        void make_tcp_query(const packet_configuration&, std::vector<uint8_t>&);
        void make_raw_query(const packet_configuration&, std::vector<uint8_t>&);

};

class ResponseFactory
{
    public:
        ResponseFactory();

        void make_packet(
            const PacketTypes&, 
            const Tins::DNS&,
            const NameUtilities::QueryProperty&, 
            std::vector<uint8_t>&
        ) const;

        static constexpr size_t udp_header_size    = 8;
        static constexpr size_t tcp_dns_size_shift = 2;

    private:
        void make_udp_response(
            const Tins::DNS&,
            const NameUtilities::QueryProperty&, 
            std::vector<uint8_t>&
        ) const;

        void make_tcp_response(
            const Tins::DNS&,
            const NameUtilities::QueryProperty&, 
            std::vector<uint8_t>&
        ) const;

        std::string form_unsuppressable_hostname(
            const int&
        ) const;

        const std::string answer_starts;
        std::vector<std::string> a_answers;
        std::vector<std::string> ns_answers;
        std::vector<std::string> mx_answers;

        std::string txt_answer;
        std::string short_txt_answer;
};

#endif
