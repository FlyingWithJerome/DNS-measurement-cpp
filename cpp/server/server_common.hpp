#ifndef SERVERCOMMONS_
#define SERVERCOMMONS_
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <boost/thread.hpp>

#include <iostream>
#include <memory>

#include <algorithm>

#include <tins/dns.h>
#include <tins/exceptions.h>

#include <sys/socket.h>
#include <sys/types.h>

#ifdef FROM_UDP_SERVER
#define UDP_ANSWERS_
#endif

#ifdef FROM_TCP_SERVER
#define TCP_ANSWERS_
#endif

#include "../constants.hpp"
#include "../packet/name_trick.hpp"
#include "../packet/response_maker.hpp"
#include "../log/log_service.hpp"

#endif
