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
#include <sstream>
#include <stdio.h>

#include <memory>

#define NAMETRICK_EXTERNAL_INCLUDE_ 1
#include "../packet/name_util.hpp"

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

typedef boost::log::sources::channel_logger_mt< > logger_type;

#define UDP_SERVER_STANDARD_LOG(fn, qp, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << "," << ep.address().to_string();}

#define UDP_SERVER_TRUNCATION_LOG(fn, qp, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << "," << ep.address().to_string();}

#define UDP_SERVER_SENDER_OVER_TCP_LOG(fn, qp, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << "," << ep.address().to_string();}

#define TCP_SERVER_STANDARD_LOG(fn, qp, sk) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id  << "," << sk.remote_endpoint().address().to_string();}

#define UDP_SERVER_MALFORM_PACKET_LOG(fn, ep) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< ep.address().to_string();}

#define UDP_SERVER_EDNS_LOG(fn, qp, ep, edns) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id           << "," \
<< ep.address().to_string() << "," \
<< edns.support_DNSSEC      << "," \
<< edns.support_EDNS0       << "," \
<< edns.support_ECS         << "," \
<< edns.EDNS0_payload       << "," \
<< edns.ECS_subnet_address  << "," \
<< edns.ECS_subnet_mask;}

#define UDP_SCANNER_NORMAL_LOG(fn, ep, qid, rcode) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qid                      << "," \
<< ep.address().to_string() << "," \
<< rcode;}

#define UDP_SCANNER_TRUNCATE_LOG(fn, ep, qid, ans_num, tc) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qid                      << "," \
<< ep.address().to_string() << "," \
<< ans_num                  << "," \
<< tc;}

#define UDP_SCANNER_BAD_RESPONSE_LOG(fn, ep, qid, rcode, jumbo, ancount, msg) {logger_type lg(boost::log::keywords::channel=fn); \
char log_entry[100]; \
sprintf( \
  log_entry, \
  "%u,%s,%d,%d,%d,%s", \
  qid, \
  ep.address().to_string().c_str(), \
  rcode, \
  jumbo, \
  ancount, \
  msg \
); \
BOOST_LOG(lg) << log_entry;}
// BOOST_LOG(lg) \
// << qid                      << std::string(",") \
// << ep.address().to_string() << std::string(",") \
// << rcode                    << std::string(",") \
// << jumbo                    << std::string(",") \
// << ancount                  << std::string(",") \
// << msg;}

#define TCP_SCANNER_NORMAL_LOG(fn, addr, qp, rcode, msg) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id                << "," \
<< addr                          << "," \
<< (int)rcode                    << "," \
<< (int)qp.jumbo_type            << "," \
<< (int)qp.normal_query_over_tcp << "," \
<< msg;}

#define TCP_SCANNER_MALFORMED_LOG(fn, addr, qp, msg) {logger_type lg(boost::log::keywords::channel=fn); \
BOOST_LOG(lg) \
<< qp.question_id                << "," \
<< addr                          << "," \
<< (int)qp.normal_query_over_tcp << "," \
<< msg;}

#endif
