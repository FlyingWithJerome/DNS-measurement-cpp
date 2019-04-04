#include "log_service.hpp"

int init_new_log_file(const char* file_name)
{
    boost::log::add_file_log(
        boost::log::keywords::file_name = file_name,
        boost::log::keywords::filter = a_channel == file_name,
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::open_mode = std::ios_base::out | std::ios_base::app
    );

    return 0;
}

int init_log_service(const int& initialize_mode)
{
    std::vector<const char*> logs;
    
    if (initialize_mode == INITIALIZE_ON_SCANNER)
    {
        logs = std::vector<const char*>{
            UDP_SCANNER_NORMAL_LOG_NAME,
            UDP_SCANNER_TRUNCATION_LOG_NAME,
            UDP_SCANNER_BAD_RESPONSE_LOG_NAME,
            TCP_SCANNER_NORMAL_LOG_NAME,
            TCP_SCANNER_BAD_RESPONSE_LOG_NAME
        };
    }
    else
    {
        logs = std::vector<const char*>{
            UDP_SERVER_NORMAL_LOG_NAME,
            UDP_SERVER_TRUNCATION_LOG_NAME,
            UDP_SERVER_SENDER_OVER_TCP_LOG_NAME,
            UDP_SERVER_MALFORM_PACKET_LOG_NAME,
            UDP_SERVER_EDNS_LOG_NAME,
            TCP_SERVER_NORMAL_LOG_NAME,
            TCP_SERVER_MALFORM_PACKET_LOG_NAME
        };
    }

    for (const char* log_name : logs)
    {
        std::cout << "[Log Service] initializing log: " << log_name << std::endl;
        init_new_log_file(log_name);
    }
    
}
