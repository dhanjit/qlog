#ifndef _FSTREAM_SYNC_LOGGER_HPP_
#define _FSTREAM_SYNC_LOGGER_HPP_

#include "SyncLogger.hpp"

namespace common {
namespace logger {
class FstreamSyncLogger : public SyncLogger<LogFile::Stream> {
   private:
    // pass
   protected:
    using parent = SyncLogger<LogFile::Stream>;
    template <char end, char delim>
    __attribute__((always_inline)) inline void lograw() {}

   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';

    template <typename... Args>
    FstreamSyncLogger(Args &&... args) : parent{std::forward<Args>(args)...} {}
    ~FstreamSyncLogger() = default;

    template <typename labellist, char end = defaultEnd, char delim = defaultDelim, typename T, typename... Args>
    __attribute__((always_inline)) inline void log(T &&t, Args &&... args) {
        if (!timestamp::is_time<T>::value) {
            this->file << timestamp::MicroSecondTime{} << delim;
        }

        using labelstringct = typename stringct::ConcatStringCT<stringct::StringCT<delim>, typename labellist::template makestr<delim>::type,
                                                                stringct::StringCT<(sizeof...(Args) == 0 ? end : delim)>>::type;
        this->file << t << labelstringct::str;
        this->lograw(std::forward<Args>(args)...);
    }

    template <char end = defaultEnd, char delim = defaultDelim, typename T, typename... Args>
    __attribute__((always_inline)) inline void lograw(T &&t, Args &&... args) {
        this->file << t << (sizeof...(Args) == 0 ? end : delim);
        this->lograw<end, delim>(std::forward<Args>(args)...);
    }
};

}    // logger end
}    // common end
#endif
