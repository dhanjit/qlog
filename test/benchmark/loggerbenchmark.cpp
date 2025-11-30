#include <benchmark/benchmark.h>
#include <fstream>
#include <iostream>
#include "MultiQueueAsyncLogger.hpp"
#include "SpscAsyncLogger.hpp"
#include "TimeStamp.hpp"

static constexpr auto maxmsgs = 64 * 8;
static constexpr auto msgsize = 64;
static constexpr auto repeat = 100000;

void spscbench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::SpscAsyncLogger<msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logMgr{"a.log", 0u};
    int a = 2, b = 5;
    double c = 5.0, d = 1.22;
    while (state.KeepRunning()) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;
        for (int i = 0; i < repeat; i++) {
            using LL = common::logger::label::LabelList<common::logger::level::INFO, common::logger::level::INFO>;
            logMgr.template log<LL>(common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        }
    }
}

void mqscbench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::MultiQueueAsyncLogger<1, msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logMgr{"b.log", 0u};
    int a = 2, b = 5;
    double c = 5.0, d = 1.22;
    while (state.KeepRunning()) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;
        for (int i = 0; i < repeat; i++) {
            using LL = common::logger::label::LabelList<common::logger::level::INFO, common::logger::level::INFO>;
            using Q0 = common::logger::QId<0>;
            logMgr.template log<LL, Q0>(common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        }
    }
}

void copybench(benchmark::State& state) {
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
            // Commented out due to compilation error with SCT("TAG") in template args
            /*
            new (buf + tail.load(std::memory_order_acquire))
                common::logger::TimedFormattedMessage<',', '\n', common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>, common::timestamp::MicroSecondTime, int, int&, int&,
                                                      double&, double&>{common::timestamp::MicroSecondTime{}, 1, a, b, c, d};
            tail = ((tail + msgsize) & (msgsize * maxmsgs - 1));
            */
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
