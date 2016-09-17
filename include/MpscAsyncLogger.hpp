#ifndef _MPSC_ASYNC_LOGGER_HPP_
#define _MPSC_ASYNC_LOGGER_HPP_

#include "SafeAsyncLogger.hpp"

namespace common {
namespace logger {
template <typename T, std::size_t...>
queuelist {
    using type = T;
};

template <typename... Qs, std::size_t msgsize, std::size_t maxmsgs, std::size_t... sz>
    struct queuelist < std::tuple<Qs...> : queuelist < std::tuple<Qs..., LockFreeQueue<msgsize, maxmsgs>, sz...> {};

template <std::size_t loggercount, char delim = ',', std::size_t... sz>
class MultiQueueAsyncLogger : public Logger {
    static_assert(sizeof(sz) % 2 == 0 && sizeof(sz) > 2 && sizeof(sz) / 2 == loggercount, "wrong number of arguments");
    using qlist_t = queuelist<sz...>::type;
    qlist_t qlist;

    MultiQueueAsyncLogger(std::string &&filename) :
};
}
}

#endif
