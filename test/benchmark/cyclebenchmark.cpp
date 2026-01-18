#include <benchmark/benchmark.h>
#include <iostream>
#include <x86intrin.h>
#include "MultiQueueAsyncLogger.hpp"
#include "SpscAsyncLogger.hpp"

// CPU cycle measurement using RDTSC
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// Serializing instruction to prevent reordering
static inline uint64_t rdtsc_start() {
    unsigned cycles_low, cycles_high;
    __asm__ __volatile__("CPUID\n\t"
                         "RDTSC\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static inline uint64_t rdtsc_end() {
    unsigned cycles_low, cycles_high;
    __asm__ __volatile__("RDTSCP\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         "CPUID\n\t"
                         : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static constexpr auto maxmsgs = 64 * 8;
static constexpr auto msgsize = 64;
static constexpr auto repeat = 100000;

// SPSC benchmark with cycle counting
void spsc_cycle_bench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::SpscAsyncLogger<msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{"alog", "a.log", 0u};

    int a = 2, b = 5;
    double c = 5.0, d = 1.22;

    uint64_t total_cycles = 0;
    uint64_t num_samples = 0;

    for (auto _ : state) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;

        // Measure cycles for a batch
        uint64_t start = rdtsc_start();
        for (int i = 0; i < repeat; i++) {
            logger.log<common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>>(
                common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        }
        uint64_t end = rdtsc_end();

        total_cycles += (end - start);
        num_samples += repeat;

        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);
        benchmark::DoNotOptimize(c);
        benchmark::DoNotOptimize(d);
    }

    state.counters["Cycles/Op"] = benchmark::Counter(
        static_cast<double>(total_cycles) / num_samples,
        benchmark::Counter::kAvgIterations);
}

// MQSC benchmark with cycle counting
void mqsc_cycle_bench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::MultiQueueAsyncLogger<1, msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{"blog", "b.log", 0u};

    int a = 2, b = 5;
    double c = 5.0, d = 1.22;

    uint64_t total_cycles = 0;
    uint64_t num_samples = 0;

    for (auto _ : state) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;

        // Measure cycles for a batch
        uint64_t start = rdtsc_start();
        for (int i = 0; i < repeat; i++) {
            logger.log<common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>, common::logger::QId<0>>(
                common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        }
        uint64_t end = rdtsc_end();

        total_cycles += (end - start);
        num_samples += repeat;

        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);
        benchmark::DoNotOptimize(c);
        benchmark::DoNotOptimize(d);
    }

    state.counters["Cycles/Op"] = benchmark::Counter(
        static_cast<double>(total_cycles) / num_samples,
        benchmark::Counter::kAvgIterations);
}

// Pure copy benchmark with cycle counting
void copy_cycle_bench(benchmark::State& state) {
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

    uint64_t total_cycles = 0;
    uint64_t num_samples = 0;

    for (auto _ : state) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;

        // Measure cycles for a batch
        uint64_t start = rdtsc_start();
        for (int i = 0; i < repeat; i++) {
            new (buf + tail.load(std::memory_order_acquire))
                common::logger::TimedFormattedMessage<',', '\n', common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>,
                                                      common::timestamp::MicroSecondTime, int, int&, int&, double&, double&>{
                    common::timestamp::MicroSecondTime{}, 1, a, b, c, d};
            tail = ((tail + msgsize) & (msgsize * maxmsgs - 1));
        }
        uint64_t end = rdtsc_end();

        total_cycles += (end - start);
        num_samples += repeat;

        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);
        benchmark::DoNotOptimize(c);
        benchmark::DoNotOptimize(d);
    }

    state.counters["Cycles/Op"] = benchmark::Counter(
        static_cast<double>(total_cycles) / num_samples,
        benchmark::Counter::kAvgIterations);
}

// Single operation benchmark - more precise
void spsc_single_op_bench(benchmark::State& state) {
    common::logger::LoggerManager<common::logger::SpscAsyncLogger<msgsize, maxmsgs, common::logger::safetypolicy::Overwrite>> logger{"clog", "c.log", 0u};

    int a = 2, b = 5;
    double c = 5.0, d = 1.22;

    std::vector<uint64_t> cycle_samples;
    cycle_samples.reserve(10000);

    for (auto _ : state) {
        a += 1;
        b += 10;
        d += 0.33;
        c += 7.01;

        // Measure single operation
        uint64_t start = rdtsc_start();
        logger.log<common::logger::label::LabelList<common::logger::level::INFO, SCT("TAG")>>(
            common::timestamp::MicroSecondTime{}, 1, a, b, c, d);
        uint64_t end = rdtsc_end();

        cycle_samples.push_back(end - start);

        benchmark::DoNotOptimize(a);
        benchmark::ClobberMemory();
    }

    // Calculate statistics
    std::sort(cycle_samples.begin(), cycle_samples.end());
    uint64_t min = cycle_samples[0];
    uint64_t max = cycle_samples[cycle_samples.size() - 1];
    uint64_t median = cycle_samples[cycle_samples.size() / 2];
    uint64_t p95 = cycle_samples[static_cast<size_t>(cycle_samples.size() * 0.95)];
    uint64_t p99 = cycle_samples[static_cast<size_t>(cycle_samples.size() * 0.99)];

    state.counters["Min_Cycles"] = min;
    state.counters["Median_Cycles"] = median;
    state.counters["P95_Cycles"] = p95;
    state.counters["P99_Cycles"] = p99;
    state.counters["Max_Cycles"] = max;
}

BENCHMARK(spsc_cycle_bench)->UseRealTime()->Iterations(100);
BENCHMARK(mqsc_cycle_bench)->UseRealTime()->Iterations(100);
BENCHMARK(copy_cycle_bench)->UseRealTime()->Iterations(100);
BENCHMARK(spsc_single_op_bench)->UseRealTime()->MinTime(1.0);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
