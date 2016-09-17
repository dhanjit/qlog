#ifndef _THREADSAFETIMESTAMP_HPP_
#define _THREADSAFETIMESTAMP_HPP_

// stdc++
#include <atomic>
#include <memory>

// Boost

// Lightening

// Custom
#include "TimeStamp.hpp"

namespace common {
namespace timestamp {
template <class PrecisionTime, std::memory_order DefaultLoadMemoryOrder = std::memory_order_relaxed,
          std::memory_order DefaultStoreMemoryOrder = std::memory_order_relaxed>
class ThreadSafeTimeStamp : public Time {
   protected:
    using IntegralType = typename PrecisionTime::IntegralType;
    std::atomic<IntegralType> t;

   public:
    ThreadSafeTimeStamp() : t{PrecisionTime{}.getIntegral()} {}
    ThreadSafeTimeStamp(const IntegralType &val) { this->t = val; }

    // No copy or move.
    ThreadSafeTimeStamp(const ThreadSafeTimeStamp &_t) = delete;
    inline ThreadSafeTimeStamp &operator=(const ThreadSafeTimeStamp &&other) = delete;
    inline ThreadSafeTimeStamp &operator=(const ThreadSafeTimeStamp &other) = delete;

    /* Setters and Assignments */
    void set(std::memory_order memoryOrder = DefaultStoreMemoryOrder) {
        PrecisionTime _t{};
        this->set(_t.getIntegral(), memoryOrder);
    }

    void set(const IntegralType &val, std::memory_order memoryOrder = DefaultStoreMemoryOrder) { this->t.store(val, memoryOrder); }

    ThreadSafeTimeStamp &operator=(const IntegralType &val) {
        this->set(val);
        return *this;
    }

    ThreadSafeTimeStamp &operator=(const PrecisionTime &_t) {
        this->set(_t.getIntegral());
        return *this;
    }

    /* Get */
    IntegralType getIntegral(std::memory_order memoryOrder = DefaultLoadMemoryOrder) const { return this->t.load(memoryOrder); }
    IntegralType getSeconds(std::memory_order memoryOrder = DefaultLoadMemoryOrder) const {
        return (this->getIntegral() / PrecisionTime::UnitsPerSec);
    }

    /* Operators overloaded. */
    __attribute__((always_inline)) inline IntegralType operator-(const ThreadSafeTimeStamp<PrecisionTime> &other) const {
        return this->getIntegral() - other.getIntegral();
    }
    __attribute__((always_inline)) inline IntegralType operator-(const IntegralType &other) const { return this->getIntegral() - other; }

    // inline bool operator< (const ThreadSafeTimeStamp<T>& other) const { return this->getIntegral() < other.getIntegral(); }
    // inline bool operator> (const ThreadSafeTimeStamp<T>& other) const { return other < this->getIntegral
    // inline bool operator<= (const ThreadSafeTimeStamp& other) const { return this->get() <= other.get(); }
    // inline bool operator>= (const ThreadSafeTimeStamp& other) const { return this->get() >= other.get(); }

    // inline bool operator< (const IntegralType& other) const { return this->get() < other; }
    // inline bool operator> (const IntegralType& other) const { return this->get() > other; }
    // inline bool operator<= (const IntegralType& other) const { return this->get() <= other; }
    // inline bool operator>= (const IntegralType& other) const { return this->get() >= other; }
};

__attribute__((always_inline)) inline MicroSecondTime::IntegralType operator-(const MicroSecondTime &lhs,
                                                                              const ThreadSafeTimeStamp<MicroSecondTime> &rhs) {
    return lhs.getIntegral() - rhs.getIntegral();
}
}
}
#endif
