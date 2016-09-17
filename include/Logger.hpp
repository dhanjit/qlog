#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <atomic>
#include <exception>
#include <fstream>
#include <ios>
#include <memory>
#include <thread>
#include <tuple>

#include "LockFreeQueue.hpp"
#include "StringCT.hpp"
#include "TimeStamp.hpp"

namespace common {
namespace logger {

namespace level {
template <uint8_t v>
struct Value {
    static constexpr uint8_t value = v;
};
struct Level {};
struct DEBUG : SCT("DBG"), Value<0>, Level {};
struct INFO : SCT("INF"), Value<1>, Level {};
struct WARN : SCT("WRN"), Value<2>, Level {};
struct ERROR : SCT("ERR"), Value<3>, Level {};
struct CRIT : SCT("CRT"), Value<4>, Level {};

static constexpr const std::size_t totalLogLevels = 5;
}

template <typename T>
struct is_level {
    static constexpr bool value = std::is_base_of<level::Level, typename std::decay<T>::type>::value;
};

namespace label {
template <typename... Args>
struct LabelList {
    template <char delim>
    struct makestr : stringct::DelimitConcatStringCT<stringct::StringCT<delim>, typename Args::type...> {};
    template <std::size_t i>
    struct get {
        using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
    };
};
}

// DD: Need to wrap info in struct. PlaceHolder should also have information what it is a placeholder for.
static const char PlaceHolder = '0';

template <typename T, int...>
struct FormattedValue;
template <typename T>
struct FormattedValue<T> {
    using value_type = typename std::decay<T>::type;

    value_type value;

    FormattedValue<T>(value_type &value_) : value{value_} {}
    FormattedValue<T>(value_type &&value_) : value{value_} {}

    value_type &&toStringify() { return std::forward<value_type>(value); }
};

template <typename T, int fixed_precision>
struct FormattedValue<T, fixed_precision> : FormattedValue<T> {
    static_assert(std::is_floating_point<typename FormattedValue<T>::value_type>::value, "This specialization only for floating point types");
    static_assert(0 <= fixed_precision && fixed_precision <= 9, "0 <= fixed_precision <= 9");

    using printfformat = typename stringct::ConcatStringCT<stringct::StringCT<'%', '.', '0' + fixed_precision>,
                                                           typename stringct::FormatSpecifierCT<typename FormattedValue<T>::value_type>::type>::type;

    FormattedValue<T, fixed_precision>(T value) : FormattedValue<T>{value} {};

    friend std::ostream &operator<<(std::ostream &os, const FormattedValue<T, fixed_precision> &fv) {
        const auto fmtflags = os.flags();
        os << std::setprecision(fixed_precision) << std::fixed;
        os << fv.value;
        os.flags(fmtflags);
        return os;
    }
};

template <typename T, int padding, int width>
struct FormattedValue<T, padding, width> : FormattedValue<T> {
    static_assert(std::is_integral<typename FormattedValue<T>::value_type>::value, "This specialization only for integral types");
    static_assert(0 <= padding && padding <= 9, "0 <= padding <= 9");

    using printfformat = typename stringct::ConcatStringCT<stringct::StringCT<'%', '0' + padding, '0' + width>,
                                                           typename stringct::FormatSpecifierCT<typename FormattedValue<T>::value_type>::type>::type;

    FormattedValue<T, padding, width>(T value) : FormattedValue<T>{value} {};

    friend std::ostream &operator<<(std::ostream &os, const FormattedValue<T, padding, width> &fv) {
        const auto fmtflags = os.flags();
        os << std::setfill((char)padding) << std::setw(width);
        os << fv.value;
        os.flags(fmtflags);
        return os;
    }
};

enum class LogFile { _Base_, Stream, Posix };

struct LoggerDefaults {
    static constexpr char defaultDelim = ',';
    static constexpr char defaultEnd = '\n';
};

template <LogFile logfile>
class Logger;

class AbstractLogger {
   protected:
    // override or keep empty;
    void start(std::string &&name) {}
    void stop() {}
};

template <>
class Logger<LogFile::Stream> : public AbstractLogger {
   protected:
    std::ofstream file;
    Logger(std::string &&filename) : file{filename, std::ios::out | std::ios::app} { this->check(); }
    ~Logger() {
        this->flush();
        this->close();
    }
    void check() {
        if (!this->file.good()) {
            throw std::ios_base::failure{"Logfile not good"};
        }
    }

   public:
    Logger(std::ofstream &file_) = delete;    // access modifier is checked before delete.
    Logger(Logger &&) = delete;
    void flush() { this->file.flush(); }
    void close() { this->file.close(); }
    std::ofstream &getFile() { return file; }
};

template <>
class Logger<LogFile::Posix> : public AbstractLogger {
   protected:
    FILE *file;
    Logger(std::string &&filename) : file{fopen(filename.c_str(), "a")} { this->check(); }
    Logger(FILE *logfile) : file{logfile} { this->check(); }
    ~Logger() {
        this->flush();
        this->close();
    }

    void check() {
        if (!this->file) {
            throw std::ios_base::failure{"Logfile not good"};
        }
    }

   public:
    Logger(Logger &&) = delete;
    void flush() { std::fflush(this->file); }
    void close() { std::fclose(this->file); }
    FILE *getFile() { return file; }
};

// Optional Helper Class: LoggerManager.

template <typename L>
class LoggerManager : public L {
   private:
    // pass

   protected:
    // pass

   public:
    void start(std::string &&name) = delete;
    void stop() = delete;

    template <typename... Args>
    LoggerManager(std::string &&name, Args &&... args) : L{std::forward<Args>(args)...} {
        this->L::start(std::forward<std::string>(name));
    }

    ~LoggerManager() { this->L::stop(); }

    // Should not be required.
    // template <typename labellist, typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    // __attribute__((always_inline)) inline void log(Args &&... args) {
    //     this->L::template log<labellist, identifiers..., end, delim>(std::forward<Args>(args)...);
    // }

    // template <typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    // __attribute__((always_inline)) inline void lograw(Args &&... args) {
    //     this->L::template lograw<indentifiers..., end, delim>(std::forward<Args>(args)...);
    // }
};

template <typename L>
class PerfLoggerManager : public LoggerManager<L> {
   private:
    using parent = LoggerManager<L>;

   public:
    template <typename... Args>
    PerfLoggerManager(Args &&... args) : parent{std::forward<Args>(args)...} {}
    ~PerfLoggerManager() = default;

    template <typename labellist, typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&... args) {
        common::timestamp::NanoSecondTime<> t1{};
        this->L::template log<labellist, identifiers..., delim, delim>(std::forward<Args>(args)...);
        common::timestamp::NanoSecondTime<> t2{};
        this->L::template lograw<identifiers..., end, delim>("LP", t2 - t1);
    }

    template <typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&... args) {
        common::timestamp::NanoSecondTime<> t1{};
        this->L::template lograw<identifiers..., delim, delim>(std::forward<Args>(args)...);
        common::timestamp::NanoSecondTime<> t2{};
        this->L::template lograw<identifiers..., end, delim>("LP", t2 - t1);
    }
};

}    // logger end
}    // common end

#endif
