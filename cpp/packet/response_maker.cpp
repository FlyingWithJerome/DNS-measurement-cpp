#include "response_maker.hpp"

// int ResponseMaker::response_maker_udp(
//     const Tins::DNS& question,
//     const NameUtilities::QueryProperty& query_property,
//     buffer_type& response_buffer
// )
// {

//     RESPONSE_MAKER_COMMON (question, query_property, response_buffer)
//     {   
//         response.rcode(DNS_RCODE_NOERROR);

//         response.truncated(query_property.will_truncate);

//         if (not query_property.will_truncate 
//             or 
//             query_property.jumbo_type == NameUtilities::JumboType::jumbo_one_answer)
        
//             response.add_answer(
//                 Tins::DNS::resource(
//                     query_property.name, // dname
//                     STANDARD_ANSWER,     // data
//                     DNS_RR_TYPE_A,       // type
//                     DNS_CLASS_IN,        // resource class
//                     DNS_RESOURCE_TTL     // time to live
//                 )
//             );
//     }
//     RESPONSE_MAKER_COMMON_END(response_buffer)

//     size_t size_of_packet = response.size(); 

//     response_buffer.reset(new uint8_t[size_of_packet]); 

//     std::vector<uint8_t>raw_data = response.serialize();

//     std::copy(raw_data.begin(), raw_data.end(), response_buffer.get());

//     return size_of_packet;
// }

// std::copy(raw_data.begin(), raw_data.end(), response_buffer.get()+2); \
uint16_t suppressed_size = (uint16_t)size_of_packet; \
*(response_buffer.get())     = suppressed_size >> 8; \
*(response_buffer.get() + 1) = suppressed_size & 0xFF;


int ResponseMaker::response_maker_tcp(
    const Tins::DNS& question,
    const NameUtilities::QueryProperty& query_property,
    buffer_type& response_buffer
)
{
    RESPONSE_MAKER_COMMON (question, query_property, response_buffer)
    {   
        response.rcode(DNS_RCODE_NOERROR);

        for(int i=0; i < NUMBER_OF_LONG_ENTRIES; i++)
        {
            response.add_answer(
                Tins::DNS::resource(
                    query_property.name,               // dname
                    ANSWER_STARTS + std::to_string(i), // data
                    DNS_RR_TYPE_A,                     // type
                    DNS_CLASS_IN,                      // resource class
                    DNS_RESOURCE_TTL                   // time to live
                )
            );
        }
    }
    RESPONSE_MAKER_COMMON_END(response_buffer)

    size_t size_of_packet = response.size();

    response_buffer.reset(new uint8_t[size_of_packet+2]); 

    std::vector<uint8_t>raw_data = response.serialize();

    std::copy(raw_data.begin(), raw_data.end(), response_buffer.get()+2);

    uint16_t suppressed_size = (uint16_t)size_of_packet;

    *(response_buffer.get())     = suppressed_size >> 8;

    *(response_buffer.get() + 1) = suppressed_size & 0xFF;

    return size_of_packet+2;
}
