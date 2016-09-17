#include "TimeStamp.hpp"

namespace common {
namespace timestamp {
__attribute__((always_inline)) static uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t)hi << 32 | lo;
}

namespace systime {
struct timedata {
    unsigned seq;

    struct { /* extract of a clocksource struct */
        int vclock_mode;
        uint64_t cycle_last;
        uint64_t mask;
        uint32_t mult;
        uint32_t shift;
    } clock;

    /* open coded 'struct timespec' */
    uint64_t wall_time_sec;
    uint64_t wall_time_snsec;
    uint64_t monotonic_time_snsec;
    uint64_t monotonic_time_sec;

    int tz_minuteswest;
    int tz_dsttime;
    // struct timezone sys_tz;
    struct timespec wall_time_coarse;
    struct timespec monotonic_time_coarse;
};

static constexpr const long vvar_address = (-10 * 1024 * 1024 - 4096);
static struct timedata const *const rawtime = (timedata *)(vvar_address + 128);

// __attribute__((noinline))
// void getWallTime(struct timespec& ts){
//    // struct timespec ts;
//    // std::memcpy(&ts, td+sizeof(td->seq)+sizeof(td->clock), sizeof(ts));
//    ts.tv_sec = td->wall_time_sec;
//    auto ns = td->wall_time_snsec;
//    ns += ((rdtsc() - td->clock.cycle_last) & td->clock.mask) * td->clock.mult;
//    ns >>= td->clock.shift;
//    ts.tv_nsec = ns;
//    // return ts;
// }

__attribute__((always_inline)) uint64_t getWallTime() {
    auto ns = rawtime->wall_time_snsec;
    ns += ((rdtsc() - rawtime->clock.cycle_last) & rawtime->clock.mask) * rawtime->clock.mult;
    ns >>= rawtime->clock.shift;
    return rawtime->wall_time_sec * 1000000000 + ns;
}
}

class SingularTimeStamp : public Time {
   private:
   protected:
    using IntegralType = uint64_t;
    using UnderlyingType = IntegralType;

    UnderlyingType t;

   public:
    static constexpr const long UnitsPerSec = 1000000000;
    SingularTimeStamp() : t{systime::getWallTime()} {}

    void set() { this->t = systime::getWallTime(); }

    UnderlyingType &getUnderlying() { return this->t; }

    const IntegralType &getIntegral() const { return this->t; }

    const IntegralType &get() const { return this->t; }

    const SecondType &getSeconds() const { return this->get() / UnitsPerSec; }

    // const MilliSecondType& getMilliSeconds() const {
    //    return (this->get() - this->getSeconds())
    // }

    const MicroSecondType &getMicroSeconds() const {}

    std::ostream operator<<
};
}
}
