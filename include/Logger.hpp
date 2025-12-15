#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>
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
}    // namespace level

template <typename T>
concept LogLevel = std::derived_from<std::decay_t<T>, level::Level>;

namespace label {
template <typename... Args>
struct LabelList {
    template <char delim>
    struct makestr : common::stringct::DelimitConcatStringCT<common::stringct::StringCT<delim>, typename Args::type...> {};
    template <std::size_t i>
    struct get {
        using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
    };
};
}    // namespace label

static const char PlaceHolder = '0';

template <typename T, int...>
struct FormattedValue;
template <typename T>
struct FormattedValue<T> {
    using value_type = std::decay_t<T>;

    value_type value;

    FormattedValue(value_type &value_) : value{value_} {}
    FormattedValue(value_type &&value_) : value{value_} {}
    FormattedValue(T value) : FormattedValue<T>(value){};

    value_type &&toStringify() { return std::forward<value_type>(value); }
};

template <std::floating_point T, int fixed_precision>
struct FormattedValue<T, fixed_precision> : FormattedValue<T> {
    static_assert(0 <= fixed_precision && fixed_precision <= 9, "0 <= fixed_precision <= 9");

    using printfformat = typename common::stringct::ConcatStringCT<common::stringct::StringCT<'%', '.', '0' + fixed_precision>,
                                                                   typename common::stringct::FormatSpecifierCT<typename FormattedValue<T>::value_type>::type>::type;

    FormattedValue(T value) : FormattedValue<T>(value){};

    friend std::ostream &operator<<(std::ostream &os, const FormattedValue<T, fixed_precision> &fv) {
        os << std::format("{:.{}f}", fv.value, fixed_precision);
        return os;
    }
};

template <std::integral T, int padding, int width>
struct FormattedValue<T, padding, width> : FormattedValue<T> {
    static_assert(0 <= padding && padding <= 9, "0 <= padding <= 9");

    using printfformat = typename common::stringct::ConcatStringCT<common::stringct::StringCT<'%', '0' + padding, '0' + width>,
                                                                   typename common::stringct::FormatSpecifierCT<typename FormattedValue<T>::value_type>::type>::type;

    FormattedValue(T value) : FormattedValue<T>(value){};

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
    void start(std::string &&name) {}
    void stop() {}
};

template <>
class Logger<LogFile::Stream> : public AbstractLogger {
   protected:
    std::ofstream file;
    Logger(std::string filename) : file{filename, std::ios::out | std::ios::app} { this->check(); }
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
    Logger(std::ofstream &file_) = delete;
    Logger(Logger &&) = delete;
    void flush() { this->file.flush(); }
    void close() { this->file.close(); }
    std::ofstream &getFile() { return file; }
};

template <>
class Logger<LogFile::Posix> : public AbstractLogger {
   protected:
    FILE *file;
    Logger(std::string filename) : file{fopen(filename.c_str(), "a")} { this->check(); }
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

template <typename L>
class LoggerManager : public L {
   private:
    // pass

   protected:
    // pass

   public:
    void start(std::string &&name) = delete;
    void stop() = delete;

   public:
    template <typename... Args>
    LoggerManager(Args &&...args) : L{std::forward<Args>(args)...} {}
    ~LoggerManager() = default;

    template <typename labellist, typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void log(Args &&...args) {
        common::timestamp::NanoSecondTime<> t1{};
        this->L::template log<labellist, identifiers..., delim, delim>(std::forward<Args>(args)...);
        common::timestamp::NanoSecondTime<> t2{};
        this->L::template lograw<identifiers..., end, delim>("LP", t2 - t1);
    }

    template <typename... identifiers, char end = L::defaultEnd, char delim = L::defaultDelim, typename... Args>
    __attribute__((always_inline)) inline void lograw(Args &&...args) {
        common::timestamp::NanoSecondTime<> t1{};
        this->L::template lograw<identifiers..., delim, delim>(std::forward<Args>(args)...);
        common::timestamp::NanoSecondTime<> t2{};
        this->L::template lograw<identifiers..., end, delim>("LP", t2 - t1);
    }
};

}    // namespace logger
}    // namespace common

#endif
