#include "packet_factory.hpp"

constexpr size_t PacketFactory::udp_header_size;
constexpr size_t PacketFactory::tcp_dns_size_shift;

void PacketFactory::make_packet(
    const PacketTypes& packet_type,  
    std::vector<uint8_t>& packet_to_be_filled, 
    const packet_configuration& packet_config
)
{
    switch (packet_type)
    {
        case UDP_QUERY:
            return make_udp_query(packet_to_be_filled, packet_config);
            break;
        
        case TCP_QUERY:
            return make_tcp_query(packet_to_be_filled, packet_config);
            break;

        case RAW_QUERY:
            return make_raw_query(packet_to_be_filled, packet_config);
            break;

        default:
            break;
    }
}


void PacketFactory::make_udp_query(
    std::vector<uint8_t>& packet_to_be_filled, 
    const packet_configuration& packet_config
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

void PacketFactory::make_tcp_query(
    std::vector<uint8_t>& packet_to_be_filled, 
    const packet_configuration& packet_config
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
    packet_to_be_filled.reserve(binary_query.size() + PacketFactory::tcp_dns_size_shift);
    uint8_t header[PacketFactory::tcp_dns_size_shift] = {
        (uint8_t)((binary_query.size() >> 8)), (uint8_t)((binary_query.size() & 0xff))
    };
    packet_to_be_filled.assign(header, header + PacketFactory::tcp_dns_size_shift);
    packet_to_be_filled.insert(
        packet_to_be_filled.begin() + PacketFactory::tcp_dns_size_shift, 
        binary_query.begin(), 
        binary_query.end()
    );
}

void PacketFactory::make_raw_query(
    std::vector<uint8_t>& packet_to_be_filled, 
    const packet_configuration& packet_config
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
    const size_t full_packet_size     = binary_query.size() + PacketFactory::udp_header_size;
    packet_to_be_filled.reserve(full_packet_size);

    const uint8_t header[PacketFactory::udp_header_size] = {
        0xb, 
        0xb7, 
        0, 
        0x35, 
        (uint8_t)(full_packet_size >> 8), 
        (uint8_t)(full_packet_size & 0xff), 
        0, 
        0
    };
    packet_to_be_filled.assign(header, header+PacketFactory::udp_header_size);
    packet_to_be_filled.insert(
        packet_to_be_filled.begin() + PacketFactory::udp_header_size, 
        binary_query.begin(), 
        binary_query.end()
    );
}
