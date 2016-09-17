#ifndef _MULTIQUEUEASYNCLOGGER_HPP_
#define _MULTIQUEUEASYNCLOGGER_HPP_

#include <algorithm>
#include <array>
#include <vector>

#include "FstreamSyncLogger.hpp"
#include "SafeAsyncLogger.hpp"

namespace common {
namespace logger {

template <std::size_t i>
struct QId {
    static constexpr std::size_t value = i;
};
//    namespace qlisttool {
//       template <std::size_t idx, std::size_t size, typename Q>
//       struct empty {
//          static bool is(Q& q) {
//             if (q.get<idx>().empty()) return );
//          }
//       };
//    }

// template <typename, std::size_t...> class MultiFixedMessageLFQ;
// template <typename... Qs> class MultiFixedMessageLFQ<std::tuple<Qs...>> {
//     using qlist_t = std::tuple<Qs...>;
//     qlist_t qlist;
//     static constexpr std::size_t qcount = sizeof(Qs...);

//     MultiFixedMessageLFQ() {}

//     template <std::size_t qnum> std::tuple_element<qnum, qlist>::type &get() { return std::get<qnum>(this->qlist); }
// };

// template <typename... Qs, std::size_t msgsize, std::size_t maxmsgs, std::size_t... sz>
// class MultiFixedMessageLFQ<std::tuple<Qs...>, msgsize,
//                            maxmsgs> : MultiFixedMessageLFQ<std::tuple<Qs..., FixedMessageLFQ<msgsize, (msgsize *maxmsgs)>>, sz...> {};

// This is actually intended to be a tuple instead of a vector/array.
// The complicacies increase manyfold when the list of queues can be of different types.
// To be done later, to include support for differently sized queues.

template <std::size_t count, std::size_t msgsize, std::size_t maxmsgs>
struct QueueList {
    using queue_t = FixedMessageLFQ<msgsize, (msgsize * maxmsgs)>;
    std::array<queue_t, count> list;
    QueueList() {}
    queue_t &operator[](std::size_t i) { return list[i]; };
    const queue_t &operator[](std::size_t i) const { return list[i]; };
};

template <std::size_t loggercnt, std::size_t msgsize, std::size_t maxmsgs, typename SafetyPolicy = safetypolicy::BackupLog<FstreamSyncLogger>>
class MultiQueueAsyncLogger : public SafeAsyncLogger<QueueList<loggercnt, msgsize, maxmsgs>, SafetyPolicy> {
   private:
    static_assert(loggercnt == 1 || !std::is_same<SafetyPolicy, safetypolicy::Overwrite>::value, "Overwrite policy only allowed if loggercnt == 1 ");

    using qlist_t = QueueList<loggercnt, msgsize, maxmsgs>;

    struct MsgToWrite {
        const Message *msg;
        timestamp::MicroSecondTime tm;
        std::size_t qid;    // qid is probably not required. This is deducible from the queue. Not done for now, due to complexity.
    };

    using time_t = decltype(MsgToWrite::tm);
    std::vector<MsgToWrite> sortedmsgs;
    std::array<time_t, loggercnt> lastTime;

   protected:
    using parent = SafeAsyncLogger<QueueList<loggercnt, msgsize, maxmsgs>, SafetyPolicy>;

   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';
    // Rant
    // Why on earth does lastTime{} <-- cause warning in gcc4.8 ?? This makes totally no sense.
    // /Rant
    // Edit: This is apparently fixed in gcc5.1
    template <typename... Args>
    MultiQueueAsyncLogger(Args &&... args) : parent{std::forward<Args>(args)...}, sortedmsgs{} {
        this->file << "0.0,[INFO], LoggerInit, MaxMsgs=" << maxmsgs << ", QSize=" << msgsize * maxmsgs << ", MsgSize=" << msgsize
                   << ", QCnt=" << loggercnt << '\n';
    }
    virtual ~MultiQueueAsyncLogger() = default;

    template <typename labellist, typename qid, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&... args) {
        static_assert(qid::value < loggercnt, "Invalid QId");
        this->parent::template log<labellist, end, delim>(this->queue[qid::value], std::forward<Args>(args)...);
    }

    template <typename qid, char end = defaultEnd, char delim = defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&... args) {
        static_assert(qid::value < loggercnt, "Invalid QId");
        this->parent::template lograw<end, delim>(this->queue[qid::value], std::forward<Args>(args)...);
    }

    void write() {
        // This can be vastly improved.
        for (std::size_t i = 0; i < loggercnt; i++) {
            auto &q = this->queue[i];
            std::size_t offset = 0;
            const auto fillsize = q.fillSize();
            while (offset < fillsize) {
                const auto &msg = q.front(offset);
                offset += msgsize;
                const auto &info = msg->getInfo();
                if (info.hasTime) {
                    lastTime[i] = *static_cast<const time_t *>(msg->getTime());
                    // sortedmsgs.emplace_back(&msg, lastTime[i], i);
                    sortedmsgs.push_back(MsgToWrite{msg, lastTime[i], i});
                } else if (offset == msgsize) {
                    sortedmsgs.push_back(MsgToWrite{msg, lastTime[i], i});
                }
            }
        }

        std::sort(sortedmsgs.begin(), sortedmsgs.end(), [](const MsgToWrite &a, const MsgToWrite &b) { return a.tm < b.tm; });

        for (auto &msg : sortedmsgs) {
            const auto &info = msg.msg->getInfo();
            if (info.isTimed && !info.hasTime) {
                this->file << msg.tm;
            }
            msg.msg->write(this->file);
            this->queue[msg.qid].pop();
            while (!this->queue[msg.qid].empty()) {
                const auto &cmsg = this->queue[msg.qid].front();
                const auto &cinfo = cmsg->getInfo();
                if (cinfo.isTimed && cinfo.hasTime) {
                    break;
                } else {
                    if (cinfo.isTimed) {
                        this->file << msg.tm;
                    }
                    cmsg->write(this->file);
                    this->queue[msg.qid].pop();
                }
            }
            // Updating head everytime has a chance of everytime refilling the head-tail cacheline.
            // But not updating head everytime might cause overflow.
        }

        // std::cerr << "fin" << std::endl;
        sortedmsgs.clear();
    }
};
}
}
#endif
