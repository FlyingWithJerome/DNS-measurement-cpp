#ifndef PACKET_FACTORY_
#define PACKET_FACTORY_

#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <tins/dns.h>
#include <ctype.h>

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

#define APPEND_ANSWER(answer_set, num) for (int answer_index = 0; answer_index < num; answer_index++) \
{ \
    response.add_answer( \
        Tins::DNS::resource( \
            query_property.name, \
            answer_set##_answers[answer_index], \
            query_type, \
            query_class, \
            DNS_RESOURCE_TTL \
        ) \
    ); \
}

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

        template <typename input_iter>
        static std::string form_txt_string(
            input_iter start,
            input_iter end
        )
        {
            std::string result;
            const int max_size = 255;
            std::string txt_segment;
            while ( start != end )
            {
                while (txt_segment.size() < max_size and start != end )
                {   
                    txt_segment += (*start++);
                }

                txt_segment.insert(0, 1, (char)txt_segment.size());
                result.append(txt_segment);
                txt_segment.clear();
            }

            std::replace(result.begin(), result.end(), '\n', ' ');
            return result;
        }

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
        std::vector<std::string> ns_answers;

        std::string txt_answer;
        std::string short_txt_answer;

        const char* ns_server_names[26] = {
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
            "X-ray",
            "Yankee",
            "Zulu"
        };
};

#endif
