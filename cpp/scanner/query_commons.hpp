#ifndef QUERY_COMMONS_
#define QUERY_COMMONS_

#include <cstring>

#define INT_TO_HEX(input_int, output_str) std::stringstream address_formatter; \
address_formatter << std::hex << input_int; \
output_str = address_formatter.str();

#define CRAFT_QUERY_PACKET_RAW(full_packet, payload) uint8_t header[8] = { \
0xb, 0xb7, 0, 0x35, (uint8_t)((payload.size() + 8) >> 8), (uint8_t)((payload.size() + 8) & 0xff), 0, 0 \
}; \
full_packet.assign(header, header+8); \
full_packet.insert(full_packet.begin()+8, payload.begin(), payload.end());

#define CRAFT_FULL_QUERY_RAW(question, packet) Tins::DNS query; \
query.id(1338); \
query.recursion_desired(1); \
query.add_query( \
    Tins::DNS::query( \
        question, \
        Tins::DNS::QueryType(Tins::DNS::QueryType::A), \
        Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET) \
    ) \
); \
std::vector<uint8_t> binary_query = query.serialize(); \
packet.reserve(binary_query.size()+8); \
CRAFT_QUERY_PACKET_RAW(packet, binary_query)

#define CRAFT_FULL_QUERY_UDP(question, packet) Tins::DNS query; \
query.id(1338); \
query.recursion_desired(1); \
query.add_query( \
    Tins::DNS::query( \
        question, \
        Tins::DNS::QueryType(Tins::DNS::QueryType::A), \
        Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET) \
    ) \
); \
packet = query.serialize();

#define CRAFT_FULL_QUERY_TCP(question, packet) Tins::DNS query; \
query.id(1338); \
query.recursion_desired(1); \
query.add_query( \
    Tins::DNS::query( \
        question, \
        Tins::DNS::QueryType(Tins::DNS::QueryType::A), \
        Tins::DNS::QueryClass(Tins::DNS::QueryClass::INTERNET) \
    ) \
); \
std::vector<uint8_t> binary_query = query.serialize(); \
packet.reserve(binary_query.size()+2); \
uint8_t header[2] = { \
    (uint8_t)((payload.size() + 8) >> 8), (uint8_t)((payload.size() + 8) & 0xff) \
}; \
packet.assign(header, header+2); \
packet.insert(packet.begin()+2, binary_query.begin(), binary_query.end());

#endif