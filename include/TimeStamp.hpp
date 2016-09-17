#ifndef _TIMESTAMP_HPP_
#define _TIMESTAMP_HPP_

// stdc++
#include <sys/time.h>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>
#include <type_traits>

// Lightening

// Custom
namespace common {
namespace timestamp {
enum class Precision { SECONDS, MILLISECONDS, MICROSECONDS, NANOSECONDS };

namespace integral {
using second = long;
using millisecond = long;
using microsecond = long;
using nanosecond = long;
}

class Time {
   protected:
    using SecondType = long;
    using MilliSecondType = int;
    using MicroSecondType = int;
    using NanoSecondType = int;

   public:
    SecondType getSeconds() const { return 0; }
    MilliSecondType getMilliSeconds() const { return 0; }
    MicroSecondType getMicroSeconds() const { return this->getMilliSeconds() * 1000; }
    NanoSecondType getNanoSeconds() const { return this->getMicroSeconds() * 1000; }

    // friend std::ostream& operator<<(std::ostream& os, const Time& t);
};

template <typename T>
struct is_time {
    static constexpr bool value = std::is_base_of<Time, typename std::decay<T>::type>::value;
};

template <class T>
using EnableForTime = typename std::enable_if<is_time<T>::value, void>::type;

class SecondTime : public Time {
   protected:
    using UnderlyingType = std::time_t;
    UnderlyingType t;

   public:
    using IntegralType = integral::second;

    static constexpr int UnitsPerSec = 1;

    SecondTime() : t{std::time(nullptr)} {}
    SecondTime(IntegralType sec) { this->t = sec; }
    SecondTime(const std::string &str, std::string &&format = "%Y-%m-%d %H:%M:%S") { this->set(str, std::forward<std::string>(format)); }
    // SecondTime(const std::string& str) { this->set(str); }

    void set() { this->t = std::time(nullptr); }
    void set(const std::string &str, std::string &&format = "%Y-%m-%d %H:%M:%S") {
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        strptime(str.c_str(), format.c_str(), &tm);
        this->t = std::mktime(&tm);
    }

    // void set(const std::string& str) { this->set(str, "%Y-%m-%d %H:%M:%S"); }

    // Probably not required.
    const SecondTime &operator=(const SecondTime &val) {
        this->t = val.getUnderlying();
        return *this;
    }
    const SecondTime &operator=(const IntegralType &val) {
        this->t = val;
        return *this;
    }

    // Getters;
    const UnderlyingType &getUnderlying() const { return this->t; }
    IntegralType getIntegral() const { return static_cast<IntegralType>(this->t); }
    SecondType getSeconds() const { return this->t; }

    SecondTime operator+(const SecondTime &rhs) const { return SecondTime{this->t + rhs.getSeconds()}; }
    SecondTime operator-(const SecondTime &rhs) const { return SecondTime{this->t - rhs.getSeconds()}; }

    bool operator==(const SecondTime &rhs) const { return this->getSeconds() == rhs.getSeconds(); }
    bool operator!=(const SecondTime &rhs) const { return !(*this == rhs); }
    bool operator<(const SecondTime &rhs) const { return this->getSeconds() < rhs.getSeconds(); }
    bool operator>(const SecondTime &rhs) const { return rhs < *this; }
    bool operator<=(const SecondTime &rhs) const { return !(*this > rhs); }
    bool operator>=(const SecondTime &rhs) const { return !(*this < rhs); }

    std::string toString(std::string &&format = "%Y-%m-%d %H:%M:%S") {
        char buf[80];
        std::strftime(buf, sizeof(buf), format.c_str(), std::localtime(&(this->t)));
        return std::string(buf);
    }

    // std::string toString() { return this->toString("%Y-%m-%d %H:%M:%S"); }
};

// DO NOT USE MilliSecondTime. Basically Useless. To use MicroSecondTime instead.
class MilliSecondTime : public Time {
   protected:
    using UnderlyingType = timeval;
    UnderlyingType t;

   public:
    using IntegralType = integral::millisecond;

    static constexpr MilliSecondType UnitsPerSec = 1000;

    MilliSecondTime() { this->set(); }
    MilliSecondTime(IntegralType millisec) { this->operator=(millisec); };

    inline void set() { gettimeofday(&this->t, NULL); }

    const MilliSecondTime &operator=(const IntegralType &val) {
        this->t.tv_sec = val / 1000;
        this->t.tv_usec = (val - this->t.tv_sec) * 1000;
        return *this;
    }

    const UnderlyingType &getUnderlying() const { return this->t; };
    IntegralType getIntegral() const { return this->t.tv_sec * 1000 + this->t.tv_usec / 1000; }
    SecondType getSeconds() const { return this->t.tv_sec; }
    MilliSecondType getMilliSeconds() const { return this->t.tv_usec / 1000; }
};

class MicroSecondTime : public Time {
   protected:
    using UnderlyingType = timeval;
    UnderlyingType t;

   public:
    using IntegralType = integral::microsecond;

    static constexpr MicroSecondType UnitsPerSec = 1000000;

    MicroSecondTime() { this->set(); }
    MicroSecondTime(IntegralType microsec) { this->operator=(microsec); };
    MicroSecondTime(const UnderlyingType &tv) { this->t = tv; };

    void set() { gettimeofday(&this->t, NULL); }

    const MicroSecondTime &operator=(const IntegralType &val) {
        this->t.tv_sec = val / 1000000;
        this->t.tv_usec = val % 1000000;
        return *this;
    }

    const UnderlyingType &getUnderlying() const { return this->t; }
    IntegralType getIntegral() const { return this->t.tv_sec * 1000000 + this->t.tv_usec; }
    SecondType getSeconds() const { return this->t.tv_sec; }
    MilliSecondType getMilliSeconds() const { return this->getMicroSeconds() / 1000; }
    MicroSecondType getMicroSeconds() const { return this->t.tv_usec; }

    IntegralType operator+(const MicroSecondTime &rhs) const {
        return (this->t.tv_sec + rhs.getSeconds()) * 1000000 + (this->t.tv_usec + rhs.getMicroSeconds());
    }

    IntegralType operator-(const MicroSecondTime &rhs) const { return this->diff(rhs.getSeconds(), rhs.getMicroSeconds()); }

    IntegralType diff(const SecondType &s, const MicroSecondType &us) const { return (this->t.tv_sec - s) * UnitsPerSec + (this->t.tv_usec - us); }

    bool operator<(const MicroSecondTime &rhs) const { return this->getIntegral() < rhs.getIntegral(); }
    bool operator>(const MicroSecondTime &rhs) const { return rhs < *this; }
    bool operator<(const SecondTime &rhs) const { return this->t.tv_sec < rhs.getSeconds(); }
    // Requires >= for > as if tv sec for microsec is = then time represented is most certainly greater.
    bool operator>(const SecondTime &rhs) const { return this->t.tv_sec >= rhs.getSeconds(); }

    // IntegralType operator+ (const MicroSecondTime& )

    // I don't want to befriend this guy, but would rather be a friend than pollute the neighbourhood.
    // Might want to templatize and use universal reference to be used for all types of 'Time', and looks be the best solution.
    // However this means polluting the neighbourhood.
    // Keeping it as is. If requirement comes would move it out. Someone has to take a hit for the greater good.

    friend std::ostream &operator<<(std::ostream &s, const MicroSecondTime &t) {
        s << t.getSeconds() << '.' << std::setfill('0') << std::setw(6) << t.getMicroSeconds();
        return s;
    }

    // template <class T, class U>
    // using isTime = typename std::enable_if<std::is_same<T,SecondTime>::value ||
    //                                        std::is_same<T,MicroSecondTime>::value ||
    //                                        std::is_same<T,MilliSecondTime>::value, U>::type;

    // Take decision on member vs non member overloads later.
    // template <class T, class U>
    // inline typename std::enable_if<isTime<T>() && isTime<U>(), bool>::type operator< (const T& a, const U& b) {
    //         if (a.getSeconds() < b.getSeconds()) return true;
    //         else if (a.getSeconds() > b.getSeconds()) return false;
    //         else if (a.getMicroSeconds() < b.getMicroSeconds()) return true;
    //         else return false;
    // }

    // template <class T, class U>
    // inline typename std::enable_if<isTime<T>() && isTime<U>(), bool>::type operator> (const T& a, const U& b) {
    //         if (a.getSeconds() > b.getSeconds()) return true;
    //         else if (a.getSeconds() < b.getSeconds()) return false;
    //         else if (a.getMicroSeconds() > b.getMicroSeconds()) return true;
    //         else return false;
    // }
};

template <clockid_t clk_id = CLOCK_REALTIME>
class NanoSecondTime : public Time {
   protected:
    using UnderlyingType = timespec;
    UnderlyingType t;

   public:
    using IntegralType = integral::nanosecond;

    static const auto UnitsPerSec = 1000000000;

    NanoSecondTime() { this->set(); }
    NanoSecondTime(IntegralType nanosec) { this->operator=(nanosec); };
    NanoSecondTime(const UnderlyingType &tv) { this->t = tv; };

    void set() { clock_gettime(clk_id, &this->t); }

    const NanoSecondTime &operator=(const IntegralType &val) {
        this->t.tv_sec = val / UnitsPerSec;
        this->t.tv_usec = val % UnitsPerSec;
        return *this;
    }

    const UnderlyingType &getUnderlying() const { return this->t; }
    IntegralType getIntegral() const { return this->t.tv_sec * UnitsPerSec + this->t.tv_nsec; }
    SecondType getSeconds() const { return this->t.tv_sec; }
    MilliSecondType getMilliSeconds() const { return this->getMicroSeconds() / 1000; }
    MicroSecondType getMicroSeconds() const { return this->getNanoSeconds() / 1000; }
    NanoSecondType getNanoSeconds() const { return this->t.tv_nsec; }

    IntegralType operator+(const NanoSecondTime &rhs) const {
        return (this->t.tv_sec + rhs.getSeconds()) * UnitsPerSec + (this->t.tv_nsec + rhs.getNanoSeconds());
    }

    IntegralType operator-(const NanoSecondTime &rhs) const { return this->diff(rhs.getSeconds(), rhs.getNanoSeconds()); }

    IntegralType diff(const SecondType &s, const NanoSecondType &ns) const { return (this->t.tv_sec - s) * UnitsPerSec + (this->t.tv_nsec - ns); }

    bool operator<(const NanoSecondTime &rhs) const { return this->getIntegral() < rhs.getIntegral(); }
    bool operator>(const NanoSecondTime &rhs) const { return rhs < *this; }
    bool operator<(const SecondTime &rhs) const { return this->t.tv_sec < rhs.getSeconds(); }
    // Requires >= for > as if tv sec for microsec is = then time represented is most certainly greater.
    bool operator>(const SecondTime &rhs) const { return this->t.tv_sec >= rhs.getSeconds(); }

    friend std::ostream &operator<<(std::ostream &s, const NanoSecondTime &t) {
        s << t.getSeconds() << '.' << std::setfill('0') << std::setw(9) << t.getNanoSeconds();
        return s;
    }
};
}
}
#endif
