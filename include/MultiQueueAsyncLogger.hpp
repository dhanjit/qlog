#ifndef _MULTIQUEUEASYNCLOGGER_HPP_
#define _MULTIQUEUEASYNCLOGGER_HPP_

#include <algorithm>
#include <array>
#include <vector>

#include "FstreamSyncLogger.hpp"
#include "LockFreeQueue.hpp"
#include "SafeAsyncLogger.hpp"

namespace common {
namespace logger {

template <std::size_t i>
struct QId {
    static constexpr std::size_t value = i;
};

template <std::size_t count, std::size_t msgsize, std::size_t maxmsgs>
struct QueueList {
    using queue_t = FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>;
    std::array<queue_t, count> list;
    QueueList() {}
    queue_t &operator[](std::size_t i) { return list[i]; };
    const queue_t &operator[](std::size_t i) const { return list[i]; };
};

struct MQMsgToWrite {
    const Message *pMsg;
    common::timestamp::MicroSecondTime timestamp;
    std::size_t qid;
};

template <std::size_t loggercnt, std::size_t msgsize, std::size_t maxmsgs, typename SafetyPolicy = safetypolicy::BackupLog<FstreamSyncLogger>>
class MultiQueueAsyncLogger : public SafeAsyncLogger<QueueList<loggercnt, msgsize, maxmsgs>, SafetyPolicy> {
   private:
    static_assert(loggercnt == 1 || !std::is_same<SafetyPolicy, safetypolicy::Overwrite>::value, "Overwrite policy only allowed if loggercnt == 1 ");

    using qlist_t = QueueList<loggercnt, msgsize, maxmsgs>;

    using time_t = common::timestamp::MicroSecondTime;
    std::vector<MQMsgToWrite> sortedmsgs;
    std::array<time_t, loggercnt> lastTime;

   protected:
    using parent = SafeAsyncLogger<QueueList<loggercnt, msgsize, maxmsgs>, SafetyPolicy>;

   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';

    template <typename... Args>
    MultiQueueAsyncLogger(Args &&...args) : parent{std::forward<Args>(args)...}, sortedmsgs{} {
        this->file << "0.0,[INFO], LoggerInit, MaxMsgs=" << maxmsgs << ", QSize=" << msgsize * maxmsgs << ", MsgSize=" << msgsize << ", QCnt=" << loggercnt << '\n';
    }
    virtual ~MultiQueueAsyncLogger() {
        if (this->workerThread.joinable()) {
            this->stop();
        }
    }

    template <typename labellist, typename qid, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&...args) {
        static_assert(qid::value < loggercnt, "Invalid QId");
        this->parent::template log<labellist, end, delim>(this->queue[qid::value], std::forward<Args>(args)...);
    }

    template <typename qid, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&...args) {
        static_assert(qid::value < loggercnt, "Invalid QId");
        this->parent::template lograw<end, delim>(this->queue[qid::value], std::forward<Args>(args)...);
    }

    void write() {
        this->sortedmsgs.clear();

        if (this->sortedmsgs.empty()) {
            return;
        }

        std::sort(this->sortedmsgs.begin(), this->sortedmsgs.end(), [](const MQMsgToWrite &a, const MQMsgToWrite &b) -> bool { return a.timestamp < b.timestamp; });

        for (auto &m : this->sortedmsgs) {
            if (m.timestamp > this->lastTime[m.qid]) {
                this->file << m.timestamp;
            }
            m.pMsg->write(this->file);
            this->queue[m.qid].pop();
            while (!this->queue[m.qid].empty()) {
                const auto *cmsg = static_cast<const Message *>(this->queue[m.qid].front());
                const auto &cinfo = cmsg->getInfo();
                if (cinfo.isTimed && cinfo.hasTime) {
                    break;
                } else {
                    if (cinfo.isTimed) {
                        this->file << m.timestamp;
                    }
                    cmsg->write(this->file);
                    this->queue[m.qid].pop();
                }
            }
        }
    }
};

}    // namespace logger
}    // namespace common
#endif
