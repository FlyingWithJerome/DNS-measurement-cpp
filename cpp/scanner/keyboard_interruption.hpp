#ifndef KEYBOARD_INTERRUPT_
#define KEYBOARD_INTERRUPT_

#include <exception>

class KeyboardInterruption : public std::exception
{
    public: 
        KeyboardInterruption(const char* msg)
        : message(msg)
        {
        }

        const char* what() const noexcept override
        {
            return message.c_str();
        }
    private:
        std::string message;
};

#endif