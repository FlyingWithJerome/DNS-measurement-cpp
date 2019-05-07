#pragma once
#ifndef DNS_PROCESS_UTIL_
#define DNS_PROCESS_UTIL_

#include <tins/dns.h>
#include "../system/lru.hpp"
#include "../log/log_service.hpp"

#define ENTRY_NOT_EXIST -1

typedef lru11::Cache<std::string, int, std::mutex> lru_cache;

std::pair<std::string, int> extract_name_type_pair(
    const Tins::DNS& dns_response
);

int query_lru(lru_cache& cache, const std::string& ip_address);

#endif