#ifndef _FPRINTF_SYNC_LOGGER_HPP_
#define _FPRINTF_SYNC_LOGGER_HPP_

#include "StringCT.hpp"
#include "SyncLogger.hpp"

namespace common {
namespace logger {
class FprintfSyncLogger : public SyncLogger<LogFile::Posix> {
   private:
    template <typename T>
    using pconvert = stringct::PrintfConvert<typename std::decay<T>::type>;

   protected:
    using parent = SyncLogger<LogFile::Posix>;
    // pass
   public:
    static constexpr auto defaultDelim = ',';
    static constexpr auto defaultEnd = '\n';

    template <typename... Args>
    FprintfSyncLogger(Args &&... args) : parent{std::forward<Args>(args)...} {}
    ~FprintfSyncLogger() = default;

    // Need a specialization for MicroSecondTime.
    // To be included along with a lograw function.
    template <typename labellist, char end = defaultEnd, char delim = defaultDelim, typename T, typename... Args>
    void log(T &&t, Args &&... args) {
        // This needs to be moved out.
        static_assert(timestamp::is_time<T>::value, "First should be Time");
        using tfmt_ct = stringct::StringCT<'%', 'l', 'd', '.', '%', '0', '6', 'd'>;

        using delimfmt_ct =
            typename stringct::DelimitConcatStringCT<typename stringct::StringCT<delim>::type, typename labellist::template makestr<delim>::type,
                                                     typename pconvert<Args>::format...>::type;
        using logfmt = typename stringct::ConcatStringCT<tfmt_ct, stringct::StringCT<delim>, delimfmt_ct, stringct::StringCT<end>>::type;
        // using logfmt = typename stringct::ConcatStringCT<typename stringct::DelimitFormatStringCT<delim,
        // decltype(gettypenameStr(labellist)),typename
        // std::decay<Args>::type..., char>::type, stringct::StringCT<'\n'> >::type;
        std::fprintf(this->file, logfmt::str, t.getSeconds(), t.getMicroSeconds(), pconvert<Args>::get(std::forward<Args>(args))...);
        // Note this breaks on volatile bool& .
    }

    template <char end = defaultEnd, char delim = defaultEnd, typename... Args>
    void lograw(Args &&... args) {
        using delimfmt_ct =
            typename stringct::DelimitConcatStringCT<typename stringct::StringCT<delim>::type, typename pconvert<Args>::format...>::type;
        using logfmt = typename stringct::ConcatStringCT<delimfmt_ct, stringct::StringCT<end>>::type;
        // using logfmt = typename stringct::ConcatStringCT<typename stringct::DelimitFormatStringCT<delim,
        // decltype(gettypenameStr(labellist)),typename
        // std::decay<Args>::type..., char>::type, stringct::StringCT<'\n'> >::type;
        std::fprintf(this->file, logfmt::str, pconvert<Args>::get(std::forward<Args>(args))...);
        // Note this breaks on volatile bool& .
    }
};

}    // logger end
}    // common end
#endif
