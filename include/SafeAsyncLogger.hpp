#ifndef _SAFE_ASYNC_LOGGER_HPP_
#define _SAFE_ASYNC_LOGGER_HPP_

#include "AsyncLogger.hpp"

namespace common {
namespace logger {
namespace safetypolicy {
struct SafetyPolicy {};
struct Ignore : public SafetyPolicy {
    template <std::size_t requiredSize, typename Q>
    static __attribute__((always_inline)) inline bool execute(Q &q) {
        // Do nothing.
        return false;
    }
};

struct Overwrite : public SafetyPolicy {
    template <std::size_t requiredSize, typename Q>
    static __attribute__((always_inline)) inline bool execute(Q &q) {
        // Allow Overwrite.
        return true;
    }
};

struct Poll : public SafetyPolicy {
    template <std::size_t requiredSize, typename Q>
    static __attribute__((always_inline)) inline bool execute(Q &q) {
        while (!q.canEnqueue(requiredSize)) {
            // Spin.
        }
        return true;
    }
};

template <typename Logger_t>
struct BackupLog : public SafetyPolicy {};

}    // safetypolicy end

template <typename queue_t, typename SafetyPolicy>
class SafeAsyncLogger : public AsyncLogger<queue_t> {
   private:
    static_assert(std::is_base_of<safetypolicy::SafetyPolicy, SafetyPolicy>::value, "Wrong Safety policy");

   protected:
    using parent = AsyncLogger<queue_t>;

    template <typename... Args>
    SafeAsyncLogger(Args &&... args) : parent(std::forward<Args>(args)...) {}
    virtual ~SafeAsyncLogger() = default;

    template <typename labellist, char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void log(Q &q, Args &&... args) {
        // __builtin_expect because this is most probably going to be true.
        if (SafetyPolicy::template execute<parent::template getRequiredSize<Q::msgSize(), labellist, end, delim, Args...>()>(q)) {
            this->parent::template log<labellist, end, delim>(q, std::forward<Args>(args)...);
        }
    }

    template <char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void lograw(Q &q, Args &&... args) {
        if (SafetyPolicy::template execute<parent::template getRequiredSize<Q::msgSize(), end, delim, Args...>()>(q)) {
            this->parent::template lograw<end, delim>(q, std::forward<Args>(args)...);
        }
    }
};

template <typename queue_t, typename L>
class SafeAsyncLogger<queue_t, safetypolicy::BackupLog<L>> : public AsyncLogger<queue_t> {
   private:
    L backupLogger;

   protected:
    using parent = AsyncLogger<queue_t>;

    template <typename T, typename... Args>
    SafeAsyncLogger(T &&filename, Args &&... args) : parent{std::forward<Args>(args)...}, backupLogger{std::forward<T>(filename)} {}

    virtual ~SafeAsyncLogger() = default;

    template <typename labellist, char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void log(Q &q, Args &&... args) {
        if (q.canEnqueue(parent::template getRequiredSize<Q::msgSize(), labellist, end, delim, Args...>())) {
            this->parent::template log<labellist, end, delim>(q, std::forward<Args>(args)...);
        } else {
            // Do something here before backing up??
            // Ideally error msg to be added at the end of args. Because scripts would work on csv columns of fields.
            // Extra field may not hurt but shifted fields would hurt badly
            this->backupLogger.log<labellist, end, delim>(std::forward<Args>(args)..., "[ALOG_ERR]", "Buffer Overflow",
                                                          parent::template getRequiredSize<Q::msgSize(), labellist, Args...>(), q.fillSize());
        }
    }

    template <char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void lograw(Q &q, Args &&... args) {
        if (q.canEnqueue(parent::template getRequiredSize<Q::msgSize(), end, delim, Args...>())) {
            this->parent::template lograw<end, delim>(q, std::forward<Args>(args)...);
        } else {
            // This needs to be a check on fillvsmax.
            // Hence now handle overflow.
            // Can't make assumptions about what is going to be logged here.
            // The most I can do is provide own timestamp with a identifier sayin it's a raw message.
            this->backupLogger.lograw<end, delim>(timestamp::MicroSecondTime{}, "RAW", std::forward<Args>(args)..., "[ALOG_ERR]", "Buffer Overflow",
                                                  parent::template getRequiredSize<Q::msgSize(), Args...>(), q.fillSize());
        }
    }
};

}    // logger End
}    // common End
#endif
