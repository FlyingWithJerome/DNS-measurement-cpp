#ifndef CONSTANTS_
#define CONSTANTS_
#define SERVER_ADDRESS "129.22.150.52"
// #define SERVER_ADDRESS "127.0.0.1"
#define DNS_PORT       53

#define AUTHORITATIVE_NAME "yumi.ipl.eecs.case.edu"
#define DELIMITER "-"
#define SIGNAL_WORD "jumbo"

#define INVALID_ID 0
#define START_WITH_0x 1

#define STANDARD_ANSWER "192.168.0.0"

#define ANSWER_STARTS "192.168.0."
#define NUMBER_OF_LONG_ENTRIES 50

#define TRUNCATION_TRICK 1

#define DNS_OPCODE_REPLY  0
#define DNS_RCODE_NOERROR 0
#define DNS_RCODE_REFUSED 5
#define DNS_DO_TRUNCATE   1
#define DNS_DONT_TRUNCATE 0
#define DNS_CLASS_IN      0x0001
#define DNS_RR_TYPE_A     1

#define DNS_RESOURCE_TTL  600


#include <memory>
#define buffer_type std::vector<uint8_t>

#define MAX_SIZE_PACKET_ACCEPT 1000

#define TCP_LOG_FILE "tcp_response.log"

#endif
