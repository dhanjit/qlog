# qlog

An extremely fast templated logging framework focused on critical path logging with **ultra-low latency** (90-181 nanoseconds). Guarantees performance equal to copy for the caller.

## Performance

**Critical Path Overhead:** **190-380 CPU cycles** (~90-181 ns @ 2.1GHz)

| Metric | CPU Cycles | Nanoseconds @ 2.1GHz |
|--------|-----------|---------------------|
| **Minimum** | **190** | **~90 ns** |
| **Median** | **380** | **~181 ns** |
| **P95** | **726** | **~346 ns** |
| **P99** | **772** | **~367 ns** |

**10-28x faster** than traditional logging methods. See [BENCHMARKS.md](BENCHMARKS.md) for detailed performance analysis.

## Features

* **Header only** - Easy integration
* **Lock-free** - Zero mutex contention
* **Zero-copy** - In-place construction via placement new
* **Both synchronous and asynchronous logging** - Synchronous is a templated wrapper over fprintf/fstream
* **Compile-time optimization** - Template metaprogramming and compile-time strings (see `StringCT`)
* **Multiple queue types** - SPSC, MPSC, Multi-Queue for different use cases
* **Best for CSV/delimiter-style** single-line logging
* **Production-ready** - Used in high-frequency trading and real-time systems

## Use Cases

**Perfect for:**
- ✅ **High-Frequency Trading (HFT)** - Nanosecond-critical trading systems
- ✅ **Real-time systems** - Hard real-time constraints
- ✅ **Low-latency microservices** - Performance-critical applications
- ✅ **Game engines** - Frame-time sensitive logging
- ✅ **Embedded systems** - Minimal overhead requirements

**When critical path performance matters more than log formatting flexibility.**

## Getting Started

### Basic Example
```cpp
#include "SpscAsyncLogger.hpp"

using namespace common::logger;

int main() {
    // Create async logger with 64-byte messages, 512 message queue
    LoggerManager<SpscAsyncLogger<64, 512>> logger{"myapp", "output.log", 0};

    // Log data - only ~190-380 CPU cycles overhead!
    int order_id = 12345;
    double price = 99.95;
    int quantity = 100;

    logger.log<LabelList<level::INFO, SCT("TRADE")>>(
        timestamp::MicroSecondTime{},
        order_id, price, quantity
    );

    // Logger automatically flushes in background thread
    return 0;
}
```

### Integration
- Add the `include` folder to your include path
- Use `LoggerManager<>` to declare the appropriate logger
- Header-only, no linking required

### Prerequisites
- gcc 4.8.3 or later (C++11 support required)
- Google Benchmark for running benchmark code

```bash
# Install Google Benchmark (Ubuntu/Debian)
sudo apt-get install libbenchmark-dev
```

## Running the Benchmarks

### Quick Start
```bash
cd test/benchmark
make clean && make

# Run standard time-based benchmarks
./loggerbenchmark

# Run CPU cycle benchmarks (more detailed)
./cyclebenchmark

# Run only single-operation benchmark for precise measurements
./cyclebenchmark --benchmark_filter=single_op
```

### Example Output
```
--------------------------------------------------------------------------------------------------------
Benchmark                                              Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------
spsc_single_op_bench/min_time:1.000/real_time       2615 ns         2505 ns       523017
    Max_Cycles=125.044k
    Median_Cycles=380
    Min_Cycles=190
    P95_Cycles=726
    P99_Cycles=772
```

### Interpreting Results
- **Min_Cycles:** Best-case performance (warm cache)
- **Median_Cycles:** Typical performance in production
- **P95/P99_Cycles:** Tail latency (use for capacity planning)
- **Max_Cycles:** Outliers (context switches, interrupts)

See [BENCHMARKS.md](BENCHMARKS.md) for comprehensive performance analysis.

## Running the Tests
```bash
cd test/benchmark
make test
```

[Unit tests TODO]

## Contributing

[TODO]

## Versioning

[TODO]

## Authors

* **Dhanjit Das** - *Initial work* - [dhanjit](https://github.com/dhanjit)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

[TODO] 

