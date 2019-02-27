#ifndef ALLOCATOR_
#define ALLOCATOR_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <boost/asio.hpp>

class handler_allocator
{
    public:
        handler_allocator();

        void* allocate(size_t);
        void deallocate(void*);

    private:
        std::aligned_storage<1024>::type storage_;
        bool currently_in_use;
};

handler_allocator::handler_allocator()
: currently_in_use(false)
{
}

void* handler_allocator::allocate(size_t size_)
{
    if (not currently_in_use and size_ < sizeof(storage_))
    {
        currently_in_use = true;
        return &storage_;
    }
    else
    {
        return ::operator new(size_);
    }
}

void handler_allocator::deallocate(void* ptr)
{
    if (ptr == &storage_)
    {
        currently_in_use = false;
    }
    else
    {
        ::operator delete(ptr);
    }
}

template <typename Handler>
class custom_alloc_handler
{
    public:
        custom_alloc_handler(handler_allocator&, Handler);

    template <typename ...Args>
    void operator()(Args&&... args);

    void operator()();

    friend void* asio_handler_allocate(size_t, custom_alloc_handler<Handler>*);
    friend void asio_handler_deallocate(void* pointer, size_t, custom_alloc_handler<Handler>*);

    private:
        handler_allocator& allocator_;
        Handler handler_;
};

template <typename Handler>
custom_alloc_handler<Handler>::custom_alloc_handler(handler_allocator& allocator, Handler handler)
: allocator_(allocator)
, handler_(handler)
{
}

template <typename Handler>
template <typename ...Args>
void custom_alloc_handler<Handler>::operator()(Args&&... args)
{
    handler_(std::forward<Args>(args)...);
}

template <typename Handler>
void custom_alloc_handler<Handler>::operator()()
{
    handler_();
}

template <typename Handler>
void* asio_handler_allocate(
    std::size_t size,
    custom_alloc_handler<Handler>* this_handler
)
{
    return this_handler->allocator_.allocate(size);
}

template <typename Handler>
void asio_handler_deallocate(
    void* pointer, 
    std::size_t,
    custom_alloc_handler<Handler>* this_handler
)
{
    this_handler->allocator_.deallocate(pointer);
}

template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_allocator& allocator, 
    Handler handler
)
{
    return custom_alloc_handler<Handler>(allocator, handler);
}

#endif
