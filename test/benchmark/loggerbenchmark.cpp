#include <benchmark/benchmark.h>
#include <iostream>
#include "MultiQueueAsyncLogger.hpp"
#include "SpscAsyncLogger.hpp"

static constexpr auto maxmsgs = 64 * 8;
static constexpr auto msgsize = 64;
static constexpr auto repeat = 100000;

void spscbench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::SpscAsyncLogger<msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{"alog", "a.log",
                                                                                                                                     0u};
    // common::logger::PerfLoggerManager<common::logger::SpscAsyncLogger<msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{"alog",
    //                                                                                                                                      "a.log",
    //                                                                                                                                      0u};
    // common::logger::SpscAsyncLogger<64, 1024*16> logger("a.log.backup", "a.log", "a.log", 100u);
    int a = 2, b = 5;
    double c = 5.0, d = 1.22;
    while (state.KeepRunning()) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;
        for (int i = 0; i < repeat; i++) {
            logger.log<common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>>(common::timestamp::MicroSecondTime{}, 1, a, b, c,
                                                                                                  d);
        }
    }
}

void mqscbench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::MultiQueueAsyncLogger<1, msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{
        "blog", "b.log", 0u};
    // common::logger::PerfLoggerManager<common::logger::MultiQueueAsyncLogger<1, msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{
    //     "blog", "b.log", 0u};
    // common::logger::MultiQueueAsyncLogger<1, msgsize, maxmsgs> logger("b.log.backup", "b.log", "b.log", 0u);
    // usleep(1000);
    int a = 2, b = 5;
    double c = 5.0, d = 1.22;
    while (state.KeepRunning()) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;
        for (int i = 0; i < repeat; i++) {
            logger.log<common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>, common::logger::QId<0>>(
                common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        }
    }
}

void copybench(benchmark::State& state) {
    // common::timestamp::MicroSecondTime x{};
    std::ofstream os{"dummy.log", std::ios::out | std::ios::app};
    std::atomic<int> head;
    std::atomic<int> tail;
    char buf[msgsize * maxmsgs];
    head = 0;
    tail = 0;
    if (!os) {
        throw std::ios_base::failure{"Logfile not good"};
    }

    int a = 2, b = 5;
    double c = 5.0, d = 1.22;

    while (state.KeepRunning()) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;
        for (int i = 0; i < 100000; i++) {
            new (buf + tail.load(std::memory_order_acquire))
                common::logger::TimedFormattedMessage<',', '\n', common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>,
                                                      common::timestamp::MicroSecondTime, int, int&, int&, double&, double&>{
                    common::timestamp::MicroSecondTime{}, 1, a, b, c, d};
            tail = ((tail + msgsize) & (msgsize * maxmsgs - 1));
        }
    }
}

BENCHMARK(spscbench)->Range(8, 8 << 10)->UseRealTime();
BENCHMARK(mqscbench)->Range(8, 8 << 10)->UseRealTime();
BENCHMARK(copybench)->Range(8, 8 << 10)->UseRealTime();

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
