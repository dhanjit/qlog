#ifndef _SPSCASYNCLOGGER_HPP_
#define _SPSCASYNCLOGGER_HPP_

#include "FstreamSyncLogger.hpp"
#include "SafeAsyncLogger.hpp"

namespace common {
namespace logger {

template <std::size_t msgsize, std::size_t maxmsgs, typename SafetyPolicy = safetypolicy::BackupLog<FstreamSyncLogger>>
class SpscAsyncLogger : public SafeAsyncLogger<FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>, SafetyPolicy> {
   private:
    timestamp::MicroSecondTime lastTime;

   protected:
    using parent = SafeAsyncLogger<FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>, SafetyPolicy>;

   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';

    template <typename... Args>
    SpscAsyncLogger(Args &&... args) : parent{std::forward<Args>(args)...}, lastTime{} {
        this->file << "0.0,[INFO], LoggerInit, MaxMsgs=" << maxmsgs << ", QSize=" << msgsize * maxmsgs << ", MsgSize=" << msgsize << '\n';
    }
    virtual ~SpscAsyncLogger() = default;

    template <typename labellist, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&... args) {
        this->parent::template log<labellist, end, delim>(this->queue, std::forward<Args>(args)...);
    }

    template <char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&... args) {
        this->parent::template lograw<end, delim>(this->queue, std::forward<Args>(args)...);
    }

    void write() {
        while (!this->queue.empty()) {
            const auto &msg = this->queue.front();
            const auto &info = msg->getInfo();
            if (info.isTimed) {
                if (info.hasTime) {
                    // There is no guarantee that this cast is even valid.
                    // This looks very very shady and dirty, but dirty deeds cause for dirty methods.
                    // This needs to be made optional through template specialization/inheritance etc...
                    this->lastTime = *(static_cast<const decltype(lastTime) *>(msg->getTime()));
                    // Requires some kind of RTTI. like maybe taking address ot time::set to uniquely identify type and storing in msginfo.
                } else {
                    this->file << this->lastTime;
                }
            }
            msg->write(this->file);
            this->queue.pop();
        }
    }
};
}
}
#endif
