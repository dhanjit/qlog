#ifndef _MULTILOGGER_HPP_
#define _MULTILOGGER_HPP_

#include "Logger.hpp"

namespace common {
namespace logger {
struct LevelList {};
struct FullLevelList : LevelList {
    template <typename Level>
    static constexpr bool exists() noexcept {
        static_assert(is_level<Level>::value, "Should be a Level");
        return true;
    }
};

struct ErrorLevelList : LevelList {
    template <typename Level>
    static constexpr bool exists() noexcept {
        static_assert(is_level<Level>::value, "Should be a Level");
        return std::is_same<Level, level::ERROR>::value || std::is_same<Level, level::CRIT>::value;
    }
};

template <typename Thresh>
struct ThresholdLevelList : LevelList {
    static_assert(is_level<Thresh>::value, "Should be a Level");
    template <typename Level>
    static constexpr bool exists() noexcept {
        static_assert(is_level<Level>::value, "Should be a Level");
        return Level::value >= Thresh::value;
    }
};

template <typename T1, typename T2, typename levelsOf1, typename levelsOf2>
class MultiLogger {
   private:
    static_assert(std::is_base_of<LevelList, levelsOf1>::value, "Should be LevelList");
    static_assert(std::is_base_of<LevelList, levelsOf2>::value, "Should be LevelList");

    T1 log1;
    T2 log2;

   public:
    template <typename F1, typename F2>
    MultiLogger(F1 &&f1, F2 &&f2) : log1(std::forward<F1>(f1)), log2(std::forward<F2>(f2)) {}

    template <typename labellist, typename... Args>
    __attribute__((always_inline)) void log(Args &&... args) {
        // Now depends on compiler optimization. which may or may not be successful.
        // A foolproof compile time implementation will be done. For now this remains as MultiLogger isn't/shouldn't be in critical path
        if (levelsOf1::template exists<typename labellist::template get<0>::type>()) {
            log1.log<labellist>(std::forward<Args>(args)...);
        }

        if (levelsOf2::template exists<typename labellist::template get<0>::type>()) {
            log2.log<labellist>(std::forward<Args>(args)...);
        }
    }

    // This will change. Needs compile time check whether flush support exists with logger.
    // Either that, or everyone should have flush support
    void flush() {
        this->log1.flush();
        this->log2.flush();
    }
};
}
}

#endif
