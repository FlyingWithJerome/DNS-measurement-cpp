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
    int         actual_num_ans;
    std::string q_name;
    
} packet_configuration;

class PacketFactory
{
    public:
        void make_packet(const PacketTypes&, const packet_configuration&, std::vector<uint8_t>&);

        static constexpr size_t udp_header_size    = 8;
        static constexpr size_t tcp_dns_size_shift = 2;

    private:
        void make_udp_query(const packet_configuration&, std::vector<uint8_t>&);
        void make_tcp_query(const packet_configuration&, std::vector<uint8_t>&);
        void make_raw_query(const packet_configuration&, std::vector<uint8_t>&);
};

#endif
