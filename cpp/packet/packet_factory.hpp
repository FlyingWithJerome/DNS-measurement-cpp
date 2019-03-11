#ifndef PACKET_FACTORY_
#define PACKET_FACTORY_

#include <algorithm>
#include <vector>
#include <tins/dns.h>

#define INT_TO_HEX(input_int, output_str) char out[20]; \
sprintf(out, "0x%x", input_int); \
output_str = out;

enum PacketTypes {
    RAW_QUERY,
    UDP_QUERY,
    UDP_RESPONSE,
    TCP_QUERY,
    TCP_RESPONSE,
};

typedef struct{
    uint32_t    id;
    int         expect_answer_count;
    int         actural_num__ans;
    std::string q_name;
} packet_configuration;

class PacketFactory
{
    public:
        PacketFactory();
        void make_packet(const PacketTypes&, std::vector<uint8_t>&, const packet_configuration&);

    private:
        void make_udp_query(std::vector<uint8_t>&, const packet_configuration&);
        void make_tcp_query(std::vector<uint8_t>&, const packet_configuration&);
        void make_raw_query(std::vector<uint8_t>&, const packet_configuration&);
};

#endif