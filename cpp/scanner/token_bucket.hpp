#ifndef TOKENBUCKET_
#define TOKENBUCKET_

#include <chrono>
#include <atomic>
#include <stdint.h>
#include <thread>

class TokenBucket
{
    public:
        TokenBucket(uint64_t rate);
        TokenBucket(const TokenBucket&) = delete;

        void consume_one_packet();

    private:
        std::atomic<uint64_t> drop_rate_;
        std::atomic<uint64_t> previous_time_;
};

#endif
