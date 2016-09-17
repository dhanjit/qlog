#ifndef _ASYNC_LOGGER_HPP_
#define _ASYNC_LOGGER_HPP_

#include <unistd.h>
#include <Logger.hpp>

namespace common {
namespace logger {

struct MessageInfo {
    char delim;
    char end;
    bool isTimed;
    bool hasTime;
    // bool isRaw = !isTimed;
};

class Message {
   public:
    virtual void write(std::ostream &os) const = 0;
    virtual MessageInfo getInfo() const = 0;
    virtual const timestamp::Time *getTime() const { return nullptr; };    // This never should be called.
};

template <std::size_t idx, std::size_t size, char delim, typename Tuple>
struct tuplewriter {
    static void write(std::ostream &os, const Tuple &t) {
        os << std::get<idx>(t);
        if (idx != size - 1) {
            os << delim;
        }
        tuplewriter<idx + 1, size, delim, Tuple>::write(os, t);
    }
};

template <std::size_t size, char delim, typename Tuple>
struct tuplewriter<size, size, delim, Tuple> {
    static void write(std::ostream &os, const Tuple &t) {}
};

// Stores Args... to tuple
// write(): eg. Args as a1,a2,a3  written to file as a1<delim>a2<delim>a3<end>
template <char delim, char end, typename... Args>
class FormattedMessage : public Message {
   private:
    // Something.
    using data_t = std::tuple<typename std::decay<Args>::type...>;
    data_t data;

   public:
    __attribute__((always_inline)) FormattedMessage(Args &&... args) : data{std::forward<Args>(args)...} {
        // Do nothing else.
    }

    using argtuple = std::tuple<Args...>;

    void write(std::ostream &os) const override {
        tuplewriter<0, sizeof...(Args), delim, decltype(this->data)>::write(os, this->data);
        os << end;
    }

    MessageInfo getInfo() const override { return MessageInfo{delim, end, false, false}; }
};

// Might end up making this a composition later if the need arises.
// Might not now that I think about it. Recheck with rawlogger/mixedlogger
// combo.
template <char delim, char end, typename labellist, typename T, typename... Args>
class TimedFormattedMessage : public TimedFormattedMessage<delim, end, labellist, void, Args...> {
   private:
    static_assert(timestamp::is_time<T>::value, "Time should be here.");

    using parent = TimedFormattedMessage<delim, end, labellist, void, Args...>;
    using time_t = typename std::decay<T>::type;
    time_t tm;

   public:
    __attribute__((always_inline)) TimedFormattedMessage(T &&tm_, Args &&... args) : parent{std::forward<Args>(args)...}, tm{std::forward<T>(tm_)} {}
    using argtuple = std::tuple<T, Args...>;

    void write(std::ostream &os) const override {
        os << this->tm;
        this->parent::write(os);
    }

    MessageInfo getInfo() const override { return MessageInfo{delim, end, true, true}; }
    const timestamp::Time *getTime() const override { return &this->tm; }
};

template <char delim, char end, typename labellist, typename... Args>
class TimedFormattedMessage<delim, end, labellist, void, Args...> : public FormattedMessage<delim, end, Args...> {
   protected:
    using parent = FormattedMessage<delim, end, Args...>;

   public:
    __attribute__((always_inline)) TimedFormattedMessage(Args &&... args) : parent(std::forward<Args>(args)...) {}
    using argtuple = std::tuple<Args...>;
    void write(std::ostream &os) const override {
        using labelstringct = typename stringct::ConcatStringCT<stringct::StringCT<delim>, typename labellist::template makestr<delim>::type,
                                                                stringct::StringCT<(sizeof...(Args) == 0 ? end : delim)>>::type;
        os << labelstringct::str;
        if (sizeof...(Args) > 0) {
            this->parent::write(os);
        }
    }

    MessageInfo getInfo() const override { return MessageInfo{delim, end, true, false}; }
};

template <std::size_t msgsize, std::size_t size>
class FixedMessageLFQ : public container::LockFreeQueue<size> {
    static_assert((msgsize & (msgsize - 1)) == 0, "msgsize should be power of 2");
    static_assert((size & (size - 1)) == 0, "size should be power of 2");

   protected:
    using parent = container::LockFreeQueue<size>;

   public:
    // Can effectively store one msg less than total.
    static constexpr std::size_t maxSize() noexcept { return size - msgsize; };

    const Message *front(int offset = 0) const { return static_cast<const Message *>(this->parent::front(offset)); }

    static constexpr std::size_t msgSize() noexcept { return msgsize; }

    void pop() { this->container::LockFreeQueue<size>::pop(msgsize); }

    template <typename T, typename... Args>
    __attribute__((always_inline)) void emplace(Args &&... args) {
        this->template doEmplace<T>(std::forward<Args>(args)...);
        this->updateTail(msgsize);
    }

    template <typename T>
    __attribute__((always_inline)) void push(const T &arg) {
        static_assert(sizeof(T) <= msgsize, "sizeof element to push must be less than msgsize");
        this->doPush(arg);
        this->updateTail(msgsize);
    }

    __attribute__((always_inline)) inline bool canEnqueue(std::size_t requiredSize) const {
        // Don't do subtraction! std::size_t
        return __builtin_expect(this->fillSize() + requiredSize < maxSize(), 1);
    }
};

namespace msgtool {
template <typename... Args>
using concatMsgList = decltype(std::tuple_cat<Args...>());

// template <typename FL, typename F, std::size_t M, typename... Args>
// struct msglist {
//     using type = FL;
// };

template <std::size_t...>
struct seq {};
template <std::size_t Start, std::size_t End, std::size_t... S>
struct genseq : genseq<Start, End - 1, End, S...> {
    static_assert(Start <= End, "Invalid Sequence");
    static_assert(End != ((std::size_t)(0) - 1), "Overflow Occurred!!");
};
template <std::size_t Start, std::size_t... S>
struct genseq<Start, Start, S...> {
    using type = seq<Start, S...>;
};

template <typename FmtMsg, std::size_t FmtMsgIdx, std::size_t ArgIdx, std::size_t CurrIdx>
struct filter {
    using elemtype = typename std::tuple_element<FmtMsgIdx, typename FmtMsg::argtuple>::type;
    template <typename... Args>
    __attribute__((always_inline)) static elemtype &&get(Args &&... args) {
        return std::forward<elemtype>(filter<FmtMsg, FmtMsgIdx, ArgIdx, CurrIdx + 1>::get(std::forward<Args>(args)...));
    }
};

template <typename FmtMsg, std::size_t FmtMsgIdx, std::size_t ArgIdx>
struct filter<FmtMsg, FmtMsgIdx, ArgIdx, ArgIdx> {
    template <typename T, typename... Args>
    __attribute__((always_inline)) static T &&get(T &&t, Args &&... args) {    // -> decltype(std::forward<typename
                                                                               // F::element<I-FS>::type>(t)) {
        return std::forward<T>(t);
    }
};

template <bool basecase, typename MsgList, std::size_t MsgListIdx, std::size_t ArgsStartIdx, typename Seq>
struct msgenqueuer;

template <typename MsgList, std::size_t MsgListIdx, std::size_t ArgsStartIdx>
struct makeEnqueuer {
   private:
    static constexpr auto msglistsz = std::tuple_size<MsgList>::value;
    static_assert(msglistsz > 0, "How can MsgList be empty? Is there no message?");

    using possibleCurrentMsg = typename std::tuple_element<(MsgListIdx < msglistsz ? MsgListIdx : MsgListIdx - 1), MsgList>::type;
    static constexpr auto argCount = std::tuple_size<typename possibleCurrentMsg::argtuple>::value;

    // seq<> for msg with no args.
    using argseq =
        typename std::conditional<(argCount > 0), typename genseq<ArgsStartIdx, ArgsStartIdx + (argCount > 0 ? argCount : 1) - 1>::type, seq<>>::type;

   public:
    using type = msgenqueuer<(msglistsz == MsgListIdx), MsgList, MsgListIdx, ArgsStartIdx, argseq>;
};

template <typename MsgList, std::size_t MsgListIdx, std::size_t ArgsStartIdx, std::size_t... S>
struct msgenqueuer<false, MsgList, MsgListIdx, ArgsStartIdx, seq<S...>> {
    using FmtMsg = typename std::tuple_element<MsgListIdx, MsgList>::type;
    template <std::size_t I>
    using filterargs = filter<FmtMsg, ArgsStartIdx, I, ArgsStartIdx>;
    template <std::size_t I>
    using typefiltered = typename std::tuple_element<I, typename FmtMsg::argtuple>::type;

    static constexpr auto MsgListSz = std::tuple_size<MsgList>::value;
    static constexpr auto FmtMsgArgCount = std::tuple_size<typename FmtMsg::argtuple>::value;

    static_assert(MsgListIdx < MsgListSz, "MsgList Idx out of Bounds");
    static_assert(sizeof...(S) == FmtMsgArgCount, "Wrong Msg or Sequence");

    template <typename Q, typename... Args>
    __attribute__((always_inline)) inline static void enqueue(Q &queue, Args &&... args) {
        // queue.template emplace<FmtMsg> (std::forward<typefiltered<S>>
        // (filterargs<S>::get(std::forward<Args>(args)...))...);
        // template <std::size_t I>
        // using std::forward<typename std::tuple_element<S,
        // std::tuple<Args>>::type>(std::get<S>(std::forward_as_tuple(args...)));
        queue.template doOffsetEmplace<FmtMsg>(MsgListIdx * Q::msgSize(), std::forward<typename std::tuple_element<S, std::tuple<Args...>>::type>(
                                                                              std::get<S>(std::forward_as_tuple(args...)))...);
        using nextEnqueuer = typename makeEnqueuer<MsgList, MsgListIdx + 1, ArgsStartIdx + FmtMsgArgCount>::type;
        nextEnqueuer::enqueue(queue, std::forward<Args>(args)...);
    }
};

template <typename MsgList, std::size_t MsgListIdx, std::size_t ArgsStartIdx, typename Seq>
struct msgenqueuer<true, MsgList, MsgListIdx, ArgsStartIdx, Seq> {
    template <typename Q, typename... Args>
    __attribute__((always_inline)) inline static void enqueue(Q &queue, Args &&... args) {
        static_assert(sizeof...(Args) == ArgsStartIdx, "Argument Index incorrect");
        queue.updateTail(std::tuple_size<MsgList>::value * Q::msgSize());
        // Do nothing.
    }
};

template <typename TupleOfMsgs, typename iFmsg, std::size_t msgsize, typename... Args>
struct msglist;
template <bool, typename TupleOfMsgs, typename iFmsg, std::size_t msgsize, typename... Args>
struct makemsglist;

// MsgList< tuple of valid msgs, current msg being created, current Argument in
// consideration, remaining arguments>
template <typename TupleOfMsgs, char delim, char end, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct msglist<TupleOfMsgs, FormattedMessage<delim, end, iArgs...>, msgsize, T, oArgs...>
    : makemsglist<(sizeof(FormattedMessage<delim, end, iArgs..., T>) > msgsize), TupleOfMsgs, FormattedMessage<delim, end, iArgs...>, msgsize, T,
                  oArgs...> {};

// MsgList for TimedFormattedmessage
template <char delim, char end, typename labellist, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct msglist<std::tuple<>, TimedFormattedMessage<delim, end, labellist, iArgs...>, msgsize, T, oArgs...>
    : makemsglist<(sizeof(TimedFormattedMessage<delim, end, labellist, iArgs..., T>) > msgsize), std::tuple<>,
                  TimedFormattedMessage<delim, end, labellist, iArgs...>, msgsize, T, oArgs...> {};

// False: Don't insert to tuple. Insert arg to current Msg.
// Do for FormattedMessage
template <typename TupleOfMsgs, char delim, char end, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct makemsglist<false, TupleOfMsgs, FormattedMessage<delim, end, iArgs...>, msgsize, T, oArgs...>
    : msglist<TupleOfMsgs, FormattedMessage<delim, end, iArgs..., T>, msgsize, oArgs...> {};
// Do for TimedFormattedMessage
template <char delim, char end, typename labellist, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct makemsglist<false, std::tuple<>, TimedFormattedMessage<delim, end, labellist, iArgs...>, msgsize, T, oArgs...>
    : msglist<std::tuple<>, TimedFormattedMessage<delim, end, labellist, iArgs..., T>, msgsize, oArgs...> {};

// True: Insert current Msg to tuple of Valid Msg. Create new FormattedMessage
// with current Arg.
// Do for FormattedMessage.
template <typename... Msgs, char delim, char end, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct makemsglist<true, std::tuple<Msgs...>, FormattedMessage<delim, end, iArgs...>, msgsize, T, oArgs...>
    : msglist<std::tuple<Msgs..., FormattedMessage<delim, delim, iArgs...>>, FormattedMessage<delim, end, T>, msgsize, oArgs...> {};
// Do for TimedFormattedMessage.
template <char delim, char end, typename labellist, typename... iArgs, std::size_t msgsize, typename T, typename... oArgs>
struct makemsglist<true, std::tuple<>, TimedFormattedMessage<delim, end, labellist, iArgs...>, msgsize, T, oArgs...>
    : msglist<std::tuple<TimedFormattedMessage<delim, delim, labellist, iArgs...>>, FormattedMessage<delim, end, T>, msgsize, oArgs...> {};

// Base Case.
template <typename... Msgs, typename Fmsg, std::size_t msgsize>
struct msglist<std::tuple<Msgs...>, Fmsg, msgsize> {
    using type = std::tuple<Msgs..., Fmsg>;
};

template <bool hasTime, char delim, char end, typename labellist, std::size_t msgsize, typename... Args>
struct tmsglisttuplebuilder {
    using type = typename msglist<std::tuple<>, TimedFormattedMessage<delim, end, labellist, void>, msgsize, Args...>::type;
};

template <char delim, char end, typename labellist, std::size_t msgsize, typename T, typename... Args>
struct tmsglisttuplebuilder<true, delim, end, labellist, msgsize, T, Args...> {
    using type = typename msglist<std::tuple<>, TimedFormattedMessage<delim, end, labellist, T>, msgsize, Args...>::type;
};

template <char delim, char end, typename labellist, std::size_t msgsize, typename... Args>
struct tmsglisttuple {
    static_assert(sizeof...(Args) == 0, "Empty case, no args should be here.");
    using type = typename tmsglisttuplebuilder<false, delim, end, labellist, msgsize, Args...>::type;
};

template <char delim, char end, typename labellist, std::size_t msgsize, typename T, typename... Args>
struct tmsglisttuple<delim, end, labellist, msgsize, T, Args...> {
    using type = typename tmsglisttuplebuilder<timestamp::is_time<T>::value, delim, end, labellist, msgsize, T, Args...>::type;
};

template <char delim, char end, std::size_t msgsize, typename T, typename... Args>
struct msglisttuple {
    using type = typename msglist<std::tuple<>, FormattedMessage<delim, end, T>, msgsize, Args...>::type;
};
}    // msgtool end

template <typename queue_t>
class AsyncLogger : public Logger<LogFile::Stream> {
   private:
    // Make a msg and keep splitting.
    // Make timedmsg
    template <typename labellist, std::size_t msgsize, char end, char delim, typename... Args>
    using MsgList = typename msgtool::tmsglisttuple<delim, end, labellist, msgsize, Args...>::type;

    // Nontimed, rawmsg.
    template <std::size_t msgsize, char end, char delim, typename... Args>
    using RawMsgList = typename msgtool::msglisttuple<delim, end, msgsize, Args...>::type;

    template <typename M>
    using enqueuer = typename msgtool::makeEnqueuer<M, 0, 0>::type;

   protected:
    using parent = Logger<LogFile::Stream>;

    // This is made a template parameter
    // static constexpr auto end = '\n';

    std::atomic<bool> stopAsync;
    std::thread asyncLogger;
    unsigned int microsleep;

    queue_t queue;

    template <std::size_t msgsize, typename labellist, char end, char delim, typename... Args>
    static constexpr std::size_t getMsgCount() noexcept {
        return std::tuple_size<MsgList<labellist, msgsize, end, delim, Args...>>::value;
    }

    template <std::size_t msgsize, char end, char delim, typename... Args>
    static constexpr std::size_t getMsgCount() noexcept {
        return std::tuple_size<RawMsgList<msgsize, end, delim, Args...>>::value;
    }

    template <std::size_t msgsize, typename labellist, char end, char delim, typename... Args>
    static constexpr std::size_t getRequiredSize() noexcept {
        return getMsgCount<msgsize, labellist, end, delim, Args...>() * msgsize;
    }

    template <std::size_t msgsize, char end, char delim, typename... Args>
    static constexpr std::size_t getRequiredSize() noexcept {
        return getMsgCount<msgsize, end, delim, Args...>() * msgsize;
    }

    AsyncLogger(std::string &&filename, unsigned int microsleep_)
        : parent{std::forward<std::string>(filename)}, stopAsync{false}, microsleep{microsleep_}, queue{} {}

    template <typename labellist, char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void log(Q &q, Args &&... args) {
        enqueuer<MsgList<labellist, Q::msgSize(), end, delim, Args...>>::enqueue(q, std::forward<Args>(args)...);
    }

    template <char end, char delim, typename Q, typename... Args>
    __attribute__((always_inline)) inline void lograw(Q &q, Args &&... args) {
        enqueuer<RawMsgList<Q::msgSize(), end, delim, Args...>>::enqueue(q, std::forward<Args>(args)...);
    }

    virtual ~AsyncLogger() {}

    // Default run thread. Ideally only write function would change in derived
    // classes.
    void run(std::string &&threadname) {
        if (const auto errornum = pthread_setname_np(this->asyncLogger.native_handle(), threadname.c_str())) {
            throw std::runtime_error("LoggerName Error: " + std::to_string(errornum));
        }

        while (!this->stopAsync.load(std::memory_order_relaxed)) {
            this->write();
            this->flush();
            if (microsleep > 0) {
                usleep(microsleep);
            }
        }
    }

    void start(std::string &&threadname) { asyncLogger = std::thread{&AsyncLogger::run, this, std::forward<std::string>(threadname)}; }

    void stop() {
        this->stopAsync = true;
        this->asyncLogger.join();
    }

    // Extra vtable solely because of this being used in run.
    virtual void write() = 0;

   public:
    // ---- commented out, not required after splitting of messages being done
    // Making a struct to check size is purely for showing the actual size vs msg
    // size in compiler error report.
    // template <std::size_t size> struct checksize : std::true_type {
    //     static_assert (size <= msgsize, "MsgSize should not be exceeeded");
    // };
};

}    // logger end
}    // common end
#endif
