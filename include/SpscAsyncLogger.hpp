#ifndef _SPSCASYNCLOGGER_HPP_
#define _SPSCASYNCLOGGER_HPP_

#include "FstreamSyncLogger.hpp"
#include "LockFreeQueue.hpp"
#include "SafeAsyncLogger.hpp"

namespace common {
namespace logger {

template <std::size_t msgsize, std::size_t maxmsgs, typename SafetyPolicy = safetypolicy::BackupLog<FstreamSyncLogger>>
class SpscAsyncLogger : public SafeAsyncLogger<FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>, SafetyPolicy> {
   private:
    using TimeType = common::timestamp::MicroSecondTime;
    TimeType myLastTime;

   protected:
    using parent = SafeAsyncLogger<FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>, SafetyPolicy>;

   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';

    template <typename... Args>
    SpscAsyncLogger(Args &&...args) : parent{std::forward<Args>(args)...}, myLastTime{} {
        this->file << "0.0,[INFO], LoggerInit, MaxMsgs=" << maxmsgs << ", QSize=" << msgsize * maxmsgs << ", MsgSize=" << msgsize << '\n';
    }
    virtual ~SpscAsyncLogger() {
        if (this->workerThread.joinable()) {
            this->stop();
        }
    }

    template <typename labellist, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&...args) {
        this->parent::template log<labellist, end, delim>(this->queue, std::forward<Args>(args)...);
    }

    template <char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&...args) {
        this->parent::template lograw<end, delim>(this->queue, std::forward<Args>(args)...);
    }

    void write() {
        while (!this->queue.empty()) {
            const auto *msg = static_cast<const Message *>(this->queue.front());
            const auto &info = msg->getInfo();
            if (info.isTimed) {
                if (info.hasTime) {
                    // There is no guarantee that this cast is even valid.
                    // This looks very very shady and dirty, but dirty deeds cause for dirty methods.
                    // This needs to be made optional through template specialization/inheritance etc...
                    this->myLastTime = *(static_cast<const decltype(myLastTime) *>(msg->getTime()));
                    // Requires some kind of RTTI. like maybe taking address ot time::set to uniquely identify type and storing in msginfo.
                } else {
                    this->file << myLastTime;
                }
            }
            msg->write(this->file);
            this->queue.pop();
        }
    }
};
}    // namespace logger
}    // namespace common
#endif
