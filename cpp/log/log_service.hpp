#ifndef LOGSERVICE_
#define LOGSERVICE_

#include <boost/shared_ptr.hpp>

#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/channel_logger.hpp>

#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include <memory>

#define TIMESTAMP std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

#define INITIALIZE_ON_SCANNER 1
#define INITIALIZE_ON_SERVER  0

#define UDP_SCANNER_NORMAL_LOG_NAME          "udp_scanner_normal.log"
#define UDP_SCANNER_TRUNCATION_LOG_NAME      "udp_scanner_truncate.log"
#define UDP_SCANNER_BAD_RESPONSE_LOG_NAME    "udp_scanner_bad_response.log"
#define UDP_SCANNER_REPEAT_RESPONSE_LOG_NAME "udp_scanner_repeat_response.log"

#define TCP_SCANNER_NORMAL_LOG_NAME       "tcp_scanner_normal.log"
#define TCP_SCANNER_BAD_RESPONSE_LOG_NAME "tcp_scanner_malformed_packet.log"

#define UDP_SERVER_NORMAL_LOG_NAME           "udp_server_normal_response.log"
#define UDP_SERVER_TRUNCATION_LOG_NAME       "udp_server_truncated_response.log"
#define UDP_SERVER_SENDER_OVER_TCP_LOG_NAME  "udp_server_sender_over_tcp.log"
#define UDP_SERVER_MALFORM_PACKET_LOG_NAME   "udp_server_malform_record.log"
#define UDP_SERVER_EDNS_LOG_NAME             "udp_server_edns_record.log"
#define TCP_SERVER_NORMAL_LOG_NAME           "tcp_server_normal_response.log"
#define TCP_SERVER_MALFORM_PACKET_LOG_NAME   "tcp_server_malform_record.log"

#define INVALID_JUMBO_TYPE     -1
#define INVALID_QUERY_TYPE     -1
#define INVALID_NUM_OF_ANSWERS -1
#define INVALID_QUESTION_ID    -1

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

int init_log_service(const int& initialize_mode);

int init_new_log_file(const char* file_name);

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(a_channel, "Channel", std::string)

typedef boost::log::sources::channel_logger_mt<std::string> logger_type;

#define UDP_SERVER_STANDARD_LOG(qp, ep, q_type) {logger_type lg(boost::log::keywords::channel=UDP_SERVER_NORMAL_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id           << "," \
<< ep.address().to_string() << "," \
<< q_type                   << "," \
<< TIMESTAMP;}

#define UDP_SERVER_TRUNCATION_LOG(qp, ep, q_type) {logger_type lg(boost::log::keywords::channel=UDP_SERVER_TRUNCATION_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id           << "," \
<< ep.address().to_string() << "," \
<< q_type                   << "," \
<< TIMESTAMP;}

#define UDP_SERVER_SENDER_OVER_TCP_LOG(qp, ep) {logger_type lg(boost::log::keywords::channel=UDP_SERVER_SENDER_OVER_TCP_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id           << "," \
<< ep.address().to_string() << "," \
<< TIMESTAMP;}

#define UDP_SERVER_MALFORM_PACKET_LOG(ep) {logger_type lg(boost::log::keywords::channel=UDP_SERVER_MALFORM_PACKET_LOG_NAME); \
BOOST_LOG(lg) \
<< ep.address().to_string() << "," \
<< TIMESTAMP;}


#define UDP_SERVER_EDNS_LOG(qp, ep, edns) {logger_type lg(boost::log::keywords::channel=UDP_SERVER_EDNS_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id           << "," \
<< ep.address().to_string() << "," \
<< edns.support_DNSSEC      << "," \
<< edns.support_EDNS0       << "," \
<< edns.support_ECS         << "," \
<< edns.EDNS0_payload       << "," \
<< edns.ECS_subnet_address  << "," \
<< edns.ECS_subnet_mask     << "," \
<< TIMESTAMP;}

#define TCP_SERVER_STANDARD_LOG(qp, sk, q_type) {logger_type lg(boost::log::keywords::channel=TCP_SERVER_NORMAL_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id                             << "," \
<< sk.remote_endpoint().address().to_string() << "," \
<< q_type                                     << "," \
<< TIMESTAMP;}

#define TCP_SERVER_MALFORM_LOG(ep) {logger_type lg(boost::log::keywords::channel=TCP_SERVER_MALFORM_PACKET_LOG_NAME); \
BOOST_LOG(lg) \
<< ep.address().to_string() << "," \
<< TIMESTAMP;}

#define UDP_SCANNER_NORMAL_LOG(ep, qid, rcode) {logger_type lg(boost::log::keywords::channel=UDP_SCANNER_NORMAL_LOG_NAME); \
BOOST_LOG(lg) \
<< qid                      << "," \
<< ep.address().to_string() << "," \
<< rcode                    << "," \
<< TIMESTAMP;}

#define UDP_SCANNER_TRUNCATE_LOG(ep, qid, ans_num, q_type, tc) {logger_type lg(boost::log::keywords::channel=UDP_SCANNER_TRUNCATION_LOG_NAME); \
BOOST_LOG(lg) \
<< qid                      << "," \
<< ep.address().to_string() << "," \
<< ans_num                  << "," \
<< q_type                   << "," \
<< tc                       << "," \
<< TIMESTAMP;}

#define UDP_SCANNER_REPEAT_RESPONSE_LOG(ep, ques) {logger_type lg(boost::log::keywords::channel=UDP_SCANNER_REPEAT_RESPONSE_LOG_NAME); \
BOOST_LOG(lg) \
<< ep.address().to_string() << "," \
<< ques                     << "," \
<< TIMESTAMP;}

#define UDP_SCANNER_BAD_RESPONSE_LOG(ep, qid, rcode, q_type, jumbo, ancount, msg) {logger_type lg(boost::log::keywords::channel=UDP_SCANNER_BAD_RESPONSE_LOG_NAME); \
BOOST_LOG(lg) \
<< qid                      << "," \
<< ep.address().to_string() << "," \
<< (int)rcode               << "," \
<< (int)q_type              << "," \
<< jumbo                    << "," \
<< ancount                  << "," \
<< msg                      << "," \
<< TIMESTAMP;}

#define TCP_SCANNER_NORMAL_LOG(addr, qp, q_type, rcode, msg) {logger_type lg(boost::log::keywords::channel=TCP_SCANNER_NORMAL_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id                << "," \
<< addr                          << "," \
<< (int)rcode                    << "," \
<< (int)q_type                   << "," \
<< (int)qp.jumbo_type            << "," \
<< (int)qp.normal_query_over_tcp << "," \
<< msg                           << "," \
<< TIMESTAMP;}

#define TCP_SCANNER_MALFORMED_LOG(addr, qp, q_type, msg) {logger_type lg(boost::log::keywords::channel=TCP_SCANNER_BAD_RESPONSE_LOG_NAME); \
BOOST_LOG(lg) \
<< qp.question_id                << "," \
<< addr                          << "," \
<< (int)q_type                   << "," \
<< (int)qp.normal_query_over_tcp << "," \
<< msg                           << "," \
<< TIMESTAMP;}

#endif
