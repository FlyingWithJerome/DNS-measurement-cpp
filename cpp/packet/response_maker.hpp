#ifndef RESPONSEMAKER_
#define RESPONSEMAKER_

#include <algorithm>
#include <boost/shared_ptr.hpp>

#include <tins/dns.h>

#define TCP_ANSWERS_ 

#include "../constants.hpp"
#include "name_trick.hpp"

#define RESPONSE_MAKER_COMMON(question, property)  Tins::DNS response; \
response.id(question.id()); \
response.type(Tins::DNS::QRType::RESPONSE); \
response.opcode(DNS_OPCODE_REPLY); \
if (property.is_authoritative) { \
for(const Tins::DNS::query& res_ : question.queries()) { \
response.add_query(res_);}

#define RESPONSE_MAKER_UDP(ques, qp) RESPONSE_MAKER_COMMON (ques, qp){ \
response.rcode(DNS_RCODE_NOERROR); \
response.truncated(qp.will_truncate); \
if (not qp.will_truncate or qp.jumbo_type == NameUtilities::JumboType::jumbo_one_answer) \
response.add_answer( \
    Tins::DNS::resource( \
        query_property.name, \
        STANDARD_ANSWER, \
        DNS_RR_TYPE_A, \
        DNS_CLASS_IN,  \
        DNS_RESOURCE_TTL \
    ) \
);} \
}else{ \
response.rcode(DNS_RCODE_REFUSED);}\
size_t size_of_packet = response.size(); \
std::vector<uint8_t>raw_data = response.serialize();


#define RESPONSE_MAKER_TCP(ques, qp)  RESPONSE_MAKER_COMMON (ques, qp){ \
response.rcode(DNS_RCODE_NOERROR); \
for(int i=0; i < NUMBER_OF_LONG_ENTRIES; i++){ \
    response.add_answer( \
        Tins::DNS::resource( \
            query_property.name, \
            std::string(ANSWER_STARTS) + std::to_string(i), \
            DNS_RR_TYPE_A, \
            DNS_CLASS_IN, \
            DNS_RESOURCE_TTL \
        ) \
    ); \
}} \
}else{ \
response.rcode(DNS_RCODE_REFUSED);}\
uint16_t size_of_packet = (uint16_t)response.size(); \
std::vector<uint8_t>raw_data = response.serialize(); \
uint8_t res_size[] = {(uint8_t)(size_of_packet >> 8), (uint8_t)(size_of_packet & 0xFF)}; \
raw_data.insert(raw_data.begin(), res_size, res_size+2);

#endif

