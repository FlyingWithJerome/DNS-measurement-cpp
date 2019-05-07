#include "dns_process_util.hpp"

std::pair<std::string, int> extract_name_type_pair(
    const Tins::DNS& dns_response
)
{
    try
    {
        int number_of_answers = std::min(
            static_cast<int>(dns_response.answers_count()), 
            static_cast<int>(dns_response.answers().size())
        );

        int number_of_queries = std::min(
            static_cast<int>(dns_response.questions_count()),
            static_cast<int>(dns_response.queries().size())
        );

        if (number_of_answers > 0)
        {
            return std::make_pair(
                dns_response.answers()[0].dname(),
                dns_response.answers()[0].query_type()
            );
        }
        else if (number_of_queries > 0)
        {
            return std::make_pair(
                dns_response.queries()[0].dname(),
                dns_response.queries()[0].query_type()
            );
        }
        else
        {
            return std::make_pair(
                std::string(),
                INVALID_QUERY_TYPE
            );
        }
    }
    catch(...)
    {
        return std::make_pair(
            std::string(),
            INVALID_QUERY_TYPE
        );
    }
}

int query_lru(lru_cache& cache, const std::string& ip_address)
{
    int number_of_packet_recv = ENTRY_NOT_EXIST;
    if (not cache.tryGet(ip_address, number_of_packet_recv))
    {
        cache.insert(ip_address, 1);
        return number_of_packet_recv;
    }
    else
    {
        cache.insert(ip_address, number_of_packet_recv+1);
        return number_of_packet_recv;
    }
}

