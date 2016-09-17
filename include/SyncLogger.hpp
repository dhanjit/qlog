#ifndef _SYNC_LOGGER_HPP_
#define _SYNC_LOGGER_HPP_

#include "Logger.hpp"
#include "StringCT.hpp"

namespace common {
namespace logger {

template <LogFile l>
class SyncLogger : public Logger<l> {
   private:
    // pass
   protected:
    using parent = Logger<l>;
    template <typename... Args>
    SyncLogger(Args &&... args) : parent{std::forward<Args>(args)...} {}
    ~SyncLogger() = default;

   public:
    // pass
};

}    // logger end
}    // common end
#endif
