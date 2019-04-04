#ifndef SYSTEM_UTILITIES_
#define SYSTEM_UTILITIES_

#include "keyboard_interruption.hpp"

int modify_sysctl_rmem(const uint32_t& rmem_size)
{
    std::ofstream rmem_config("/proc/sys/net/core/rmem_max");
    if (rmem_config.is_open())
    {
        rmem_config << std::to_string(rmem_size) << "\n";
        return 0;
    }
    return -1;
}

void keyboard_interruption_handler_child(int signal)
{
    std::cout << "[Scanner General] Going to Exit (Child)...\n";
    throw KeyboardInterruption("interrupt child");
}

void keyboard_interruption_handler_parent(int signal)
{
    std::cout << "[Scanner General] Going to Exit (Parent)...\n";
    throw KeyboardInterruption("interrupt parent");
}

#define REGISTER_INTERRUPTION(identity) struct sigaction interruption_handler; \
interruption_handler.sa_handler = keyboard_interruption_handler_##identity; \
interruption_handler.sa_flags   = 0; \
sigemptyset(&interruption_handler.sa_mask); \
sigaction(SIGINT, &interruption_handler, nullptr);

#endif
