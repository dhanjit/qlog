#ifndef _LOCK_FREE_QUEUE_HPP_
#define _LOCK_FREE_QUEUE_HPP_

#include <atomic>
#include <memory>
#include <new>

#include <concepts>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace common {
namespace container {

template <std::size_t size>
class LockFreeQueue {
   private:
    alignas(hardware_destructive_interference_size) std::atomic<std::size_t> head;
    alignas(hardware_destructive_interference_size) std::atomic<std::size_t> tail;
    alignas(hardware_destructive_interference_size) char buffer[size];

   public:
    template <typename T>
    __attribute__((always_inline)) void doPush(const T &arg) {
        // reinterpret_cast is needed because buffer is char array
        T *storeAt = reinterpret_cast<T *>(this->buffer + this->tail.load(std::memory_order_acquire));
        *storeAt = arg;
    }

    template <typename T, typename... Args>
    __attribute__((always_inline)) void doEmplace(Args &&...args) {
        new (this->buffer + this->tail.load(std::memory_order_acquire)) T{std::forward<Args>(args)...};
    }

    template <typename T, typename... Args>
    __attribute__((always_inline)) inline void doOffsetEmplace(std::size_t offset, Args &&...args) {
        new (this->buffer + ((this->tail.load(std::memory_order_relaxed) + offset) & (size - 1))) T{std::forward<Args>(args)...};
    }

    __attribute__((always_inline)) void updateTail(std::size_t elemsize) { this->tail.store((this->tail.load(std::memory_order_relaxed) + elemsize) & (size - 1), std::memory_order_release); }

    __attribute__((always_inline)) void updateHead(std::size_t elemsize) { this->head.store((this->head.load(std::memory_order_relaxed) + elemsize) & (size - 1), std::memory_order_release); }

   public:
    LockFreeQueue() : head{0}, tail{0} { static_assert((size & (size - 1)) == 0, "size should be power of 2"); }
    ~LockFreeQueue() {}
    LockFreeQueue(LockFreeQueue &&) = delete;
    LockFreeQueue operator=(LockFreeQueue &&) = delete;

    template <typename T, typename... Args>
    __attribute__((always_inline)) void emplace(Args &&...args) {
        this->doEmplace<T>(std::forward<Args>(args)...);
        this->updateTail(sizeof(T));
    }

    template <typename T>
    __attribute__((always_inline)) void push(const T &arg) {
        this->doPush(arg);
        this->updateTail(sizeof(T));
    }

    std::size_t fillSize() const { return ((size + this->tail.load(std::memory_order_relaxed) - this->head.load(std::memory_order_relaxed)) & (size - 1)); }

    bool empty() const { return this->head.load(std::memory_order_acquire) == this->tail.load(std::memory_order_acquire); }

    const void *front(std::size_t offset = 0) const { return this->buffer + ((this->head.load(std::memory_order_acquire) + offset) & (size - 1)); }

    __attribute__((always_inline)) void pop(std::size_t popsize) { this->updateHead(popsize); }

    std::size_t getHead(std::memory_order mo = std::memory_order_relaxed) const { return this->head.load(mo); }
    std::size_t getTail(std::memory_order mo = std::memory_order_relaxed) const { return this->tail.load(mo); }
};
}    // namespace container
}    // namespace common

#endif
