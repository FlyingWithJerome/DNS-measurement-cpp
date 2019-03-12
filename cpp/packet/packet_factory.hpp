#ifndef PACKET_FACTORY_
#define PACKET_FACTORY_

#include <algorithm>
#include <vector>
#include <tins/dns.h>

#include "name_util.hpp"
#include "../constants.hpp"

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

typedef struct{
    uint32_t    id;
    uint16_t    query_type;
    int         expect_answer_count;
    int         actual_num_ans;
    std::string q_name;
    
} packet_configuration;

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

        const std::string answer_starts;
        std::vector<std::string> tcp_answers;
};

#endif
