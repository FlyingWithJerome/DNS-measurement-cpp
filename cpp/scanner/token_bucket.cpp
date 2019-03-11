#include "token_bucket.hpp"

TokenBucket::TokenBucket(uint64_t rate)
: drop_rate_(
    1000000 / rate
)
, previous_time_(
    0
)
{
}

void TokenBucket::consume_one_packet()
{
    uint64_t time_now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    uint64_t old_time  = previous_time_.load();

    uint64_t time_from_previous = time_now - old_time;

    if (old_time == 0 or time_from_previous >= drop_rate_.load())
    {
        previous_time_ = time_now;
        return;
    }

    std::this_thread::sleep_for(
        std::chrono::microseconds(
            time_from_previous - drop_rate_.load()
        )
    );
}
