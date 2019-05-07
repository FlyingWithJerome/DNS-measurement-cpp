typedef struct {
    bool is_ipv6 : 1,
    int src_port : 8,
    int dst_port : 8,
    int c_sum    : 8,
    uint64_t len : 64,
} custom_packet;

int get_dst_port(const char* raw_packet)
{
    UDP* udp_packet = (UDP*)raw_packet;
    return udp_packet -> dst_port;
}


