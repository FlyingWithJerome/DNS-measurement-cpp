#define BOOST_ALL_DYN_LINK

#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;

struct Logger {
    void init(bool is_verbose) {
        namespace keywords = boost::log::keywords;
        namespace sinks    = boost::log::sinks;
        namespace trivial  = boost::log::trivial;

        if(!is_verbose)
            logging::core::get()->set_filter(trivial::severity >= trivial::warning);   

        logging::add_file_log(
                keywords::auto_flush          = true,
                keywords::file_name           = is_verbose? "verbose_%N.log":"sample_%N.log",
                keywords::rotation_size       = 10 * 1024 * 1024,
                keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
                keywords::format              = "[%TimeStamp%][%Severity%]: %Message%"
            );

        logging::add_common_attributes();
    }
};

int main(int argc, char**) {
    Logger logger;
    logger.init(argc>1);

    {
        using sl = boost::log::trivial::severity_level;
        logging::sources::severity_logger<sl> lg;

        BOOST_LOG_SEV(lg, sl::trace) << "trace";
        BOOST_LOG_SEV(lg, sl::fatal) << "fatal";
    }

    BOOST_LOG_TRIVIAL(trace) << "trivial trace";
    BOOST_LOG_TRIVIAL(fatal) << "trivial fatal";
}
