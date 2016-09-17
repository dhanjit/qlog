#ifndef _LOCK_FREE_QUEUE_HPP_
#define _LOCK_FREE_QUEUE_HPP_

#include <atomic>
#include <memory>

namespace common {
namespace container {
template <std::size_t size>
class LockFreeQueue {
   private:
    std::atomic<int> head;
    // char head_padding[128];
    std::atomic<int> tail;
    // char tail_padding[128];
    char buffer[size];
    // char *buffer = new char[size];
    // char buffer_padding[128];

   protected:
    template <typename T>
    __attribute__((always_inline)) void doPush(const T &arg) {
        const T *storeAt = this->buffer + this->tail.load(std::memory_order_acquire);
        *storeAt = arg;
    }

    template <typename T, typename... Args>
    __attribute__((always_inline)) void doEmplace(Args &&... args) {
        new (this->buffer + this->tail.load(std::memory_order_acquire)) T{std::forward<Args>(args)...};
    }

    __attribute__((always_inline)) void updateTail(std::size_t elemsize) { this->tail = ((this->tail + elemsize) & (size - 1)); }

    __attribute__((always_inline)) void updateHead(std::size_t elemsize) { this->head = ((this->head + elemsize) & (size - 1)); }

   public:
    LockFreeQueue() : head{0}, tail{0} { static_assert((size & (size - 1)) == 0, "size should be power of 2"); }
    ~LockFreeQueue() {}
    LockFreeQueue(LockFreeQueue &&) = delete;
    LockFreeQueue operator=(LockFreeQueue &&) = delete;

    template <typename T, typename... Args>
    __attribute__((always_inline)) void emplace(Args &&... args) {
        this->doEmplace<T>(std::forward<Args>(args)...);
        this->updateTail(sizeof(T));
    }

    template <typename T>
    __attribute__((always_inline)) void push(const T &arg) {
        this->doPush(arg);
        this->updateTail(sizeof(T));
    }

    // This is just a guess.
    std::size_t fillSize() const { return ((size + this->tail - this->head) & (size - 1)); }

    bool empty() const { return this->head.load(std::memory_order_acquire) == this->tail.load(std::memory_order_acquire); }

    const void *front(int offset = 0) const { return this->buffer + ((this->head.load(std::memory_order_acquire) + offset) & (size - 1)); }

    __attribute__((always_inline)) void pop(std::size_t popsize) { this->updateHead(popsize); }

    // Exposed mostly for debugging. Shouldn't be required elsewhere.
    int getHead(std::memory_order mo = std::memory_order_relaxed) const { return this->head.load(mo); }
    int getTail(std::memory_order mo = std::memory_order_relaxed) const { return this->tail.load(mo); }
};
}
}

#endif
