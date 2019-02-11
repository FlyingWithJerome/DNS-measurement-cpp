#include "log_service.hpp"

int init_new_log_file(const char* file_name)
{
    boost::log::add_file_log(
        boost::log::keywords::file_name = file_name,
        boost::log::keywords::filter = a_channel == file_name,
        boost::log::keywords::auto_flush = true
    );

    return 0;
}

int init_log_service()
{
    // boost::log::formatter fmt = boost::log::expressions::stream
    // << severity << " " << boost::log::expressions::smessage;

    boost::log::add_file_log(
        boost::log::keywords::file_name = "tcp_a.log",
        boost::log::keywords::filter = a_channel == "A",
        boost::log::keywords::auto_flush = true
    );

    boost::log::add_file_log(
        boost::log::keywords::file_name = "tcp_b.log",
        boost::log::keywords::filter = a_channel == "B",
        boost::log::keywords::auto_flush = true
    );

    // boost::log::core::get()->set_filter(a_channel == "A");

    logger_type lg_a(boost::log::keywords::channel="A");
    logger_type lg_b(boost::log::keywords::channel="B");

    BOOST_LOG(lg_a) << "Hello, A.log! (TEST)";
    BOOST_LOG(lg_b) << "Hello, B.log! (TEST)";
}

// bool my_filter(boost::log::value_ref< severity_level, tag::severity > const& level)
// {
//     std::cout << "level value " << level << std::endl;
//     return level == severity_level::warning;
// }

// std::ostream& operator<< (std::ostream& strm, severity_level level)
// {
//     static const char* strings[] =
//     {
//         "trace",
//         "debug",
//         "info",
//         "warning",
//         "error",
//         "critical"
//     };

//     if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
//         strm << strings[level];
//     else
//         strm << static_cast< int >(level);

//     return strm;
// };
