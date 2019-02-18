#ifndef QUERY_COMMONS_
#define QUERY_COMMONS_


#define INT_TO_HEX(input_int, output_str) std::stringstream address_formatter; \
address_formatter << std::hex << input_int; \
output_str = address_formatter.str();

#define CRAFT_QUERY_PACKET(full_packet, payload) full_packet[0] = local_port_num >> 8; \
full_packet[1] = local_port_num & 0xff; \
full_packet[2] = remote_port_num >> 8; \
full_packet[3] = remote_port_num & 0xff; \
full_packet[4] = payload.size() >> 8; \
full_packet[5] = payload.size() & 0xff; \
full_packet[6] = 0; \
full_packet[7] = 0; \
full_packet.insert(std::end(full_packet), std::begin(payload), std::end(payload));

#define CRAFT_FULL_QUERY(question, packet) Tins::DNS query; \
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
CRAFT_QUERY_PACKET(packet, binary_query)

#endif