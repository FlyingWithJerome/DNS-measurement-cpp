#ifndef EDNS_
#define EDNS_

#include <iostream>
#include <tins/dns.h>

#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct EDNS
{
    EDNS(const Tins::DNS& incoming_packet);

    bool support_DNSSEC;
    bool support_EDNS0;
    bool support_ECS;

    std::string ECS_subnet_address;
    int ECS_subnet_mask;

    int EDNS0_payload;
};

#endif
