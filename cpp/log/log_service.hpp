#ifndef LOGSERVICE_
#define LOGSERVICE_

#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/channel_logger.hpp>

#include <iostream>
#include <fstream>

#include <memory>


#define NAMETRICK_EXTERNAL_INCLUDE_ 1
#include "../packet/name_trick.hpp"

enum severity_level 
{
  trace,
  debug,
  info,
  warning,
  error,
  fatal,
  report
};

int init_log_service();

int init_new_log_file(const char* file_name);

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(a_channel, "Channel", std::string)

typedef boost::log::sources::channel_logger< > logger_type;

#define UDP_STANDARD_LOG(fn, qp, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << std::string(",") << ep.address().to_string();}

#define UDP_TRUNCATION_LOG(fn, qp, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << std::string(",") << ep.address().to_string();}

#define TCP_STANDARD_LOG(fn, qp, sk) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << std::string(",") << sk.remote_endpoint().address().to_string();}

#define MALFORM_PACKET_UDP_LOG(fn, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< "Malformed Packet," << ep.address().to_string();
}

#define EDNS_LOG(fn, qp, ep, edns) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id           << std::string(",") \
<< ep.address().to_string() << std::string(",") \
<< edns.support_DNSSEC      << std::string(",") \
<< edns.support_EDNS0       << std::string(",") \
<< edns.support_ECS         << std::string(",") \
<< edns.EDNS0_payload       << std::string(",") \
<< edns.ECS_subnet_address  << std::string(",") \
<< edns.ECS_subnet_mask;}

// bool my_filter(boost::log::value_ref< severity_level, tag::severity > const& level);

// std::ostream& operator<< (std::ostream& strm, severity_level level);
#endif
