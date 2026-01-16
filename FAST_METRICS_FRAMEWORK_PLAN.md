# Fast Metrics Framework - Implementation Plan

**Project:** Ultra-Low Latency Metrics Collection Framework for HFT/Algorithmic Trading
**Based On:** QLOG fast logging framework (lock-free queue architecture)
**Date Created:** 2026-01-16
**Target Repository:** New separate repository (not qlog)

---

## Executive Summary

Build a metrics collection framework based on QLOG's lock-free queue architecture that provides:
- **10-20ns overhead** on critical path (vs 300-500ns for OpenTelemetry)
- **Microsecond-resolution** data collection
- **Multi-tier storage** architecture for different time scales
- **Hybrid stack** supporting both custom and standard tools (Prometheus/Grafana)

### Target Market

**Primary Target: Group 2 - Software HFT / Market Making**
- 500-1,000 firms globally
- $2-5B infrastructure market
- Latency budget: 100ns - 10μs
- **Need custom stack** - Prometheus/Grafana too slow for critical path
- Market: Citadel Securities, Virtu Financial, Flow Traders, Optiver, etc.

**Secondary Target: Group 3 - Low-Latency Algorithmic Trading**
- 5,000-10,000 firms globally
- $2-3B infrastructure market
- Latency budget: 10-100μs
- **Can piggyback on Prometheus/Grafana** for most use cases
- Market: Two Sigma, DE Shaw, WorldQuant, smaller quant funds

**Total Addressable Market:** $300M ARR potential

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│ CRITICAL PATH: Trading/Processing Logic                     │
│   ↓ every 2μs (configurable)                               │
│ Lock-Free Queue (QLOG-based) - 10-20ns overhead            │
└─────────────────────────────────────────────────────────────┘
                    ↓ (Background thread writes)
┌─────────────────────────────────────────────────────────────┐
│ TIER 1: Raw Binary Storage (microsecond granularity)        │
│ - Format: Memory-mapped binary files                        │
│ - Retention: Last 1-60 minutes                              │
│ - Use: Forensic analysis, debugging specific events         │
│ - Target: Group 2 (Software HFT)                            │
└─────────────────────────────────────────────────────────────┘
                    ↓ (Aggregate every 1 second)
┌─────────────────────────────────────────────────────────────┐
│ TIER 2: Time-Series Database (second-level aggregates)      │
│ - Options: QuestDB (preferred), InfluxDB, TimescaleDB       │
│ - Metrics: min/max/avg/p50/p95/p99/stddev per second       │
│ - Retention: 24-72 hours                                    │
│ - Use: Near-real-time dashboards (custom)                   │
│ - Target: Both Group 2 & 3                                  │
└─────────────────────────────────────────────────────────────┘
                    ↓ (Aggregate every 15-60 seconds)
┌─────────────────────────────────────────────────────────────┐
│ TIER 3: Prometheus + Grafana (minute-level aggregates)      │
│ - Metrics: min/max/avg per minute                           │
│ - Retention: 7-30 days                                      │
│ - Use: Standard monitoring dashboards                       │
│ - Target: Group 3 (ops teams for Group 2)                   │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components to Build

### Component 1: Lock-Free Metrics Queue (Core)

**Based on:** `LockFreeQueue.hpp` from QLOG
**Technology:** C++17/20, header-only library
**Key Features:**
- Single Producer Single Consumer (SPSC) variant
- Multi Producer Single Consumer (MPSC) variant
- Cache-line aligned atomics (64-byte alignment)
- In-place construction via placement new
- Compile-time metric name validation (StringCT)

**API Design:**
```cpp
// Usage example
MetricsCollector<msgsize=64, qsize=1024*1024> metrics;

// Critical path - 10-20ns
metrics.record<"TradeLatency">(timestamp_ns, price, quantity, latency_ns);
metrics.counter<"OrdersSent">()++;
metrics.gauge<"QueueDepth">() = current_depth;
metrics.histogram<"FillSize">().record(quantity);
```

**Files to Create:**
- `include/MetricsQueue.hpp` - Core lock-free queue
- `include/MetricMessage.hpp` - Message types (counter, gauge, histogram, timer)
- `include/MetricsCollector.hpp` - High-level API
- `include/StringCT.hpp` - Compile-time string processing (adapted from QLOG)

---

### Component 2: Binary Storage Writer (Tier 1)

**Purpose:** Write raw microsecond-resolution data to memory-mapped files
**Target:** Group 2 (Software HFT) only

**Features:**
- Memory-mapped file I/O
- Rolling file management (hourly rotation)
- Compact binary format (16-22 bytes per sample)
- Async flush (msync)

**Binary Format:**
```cpp
struct __attribute__((packed)) MetricRecord {
    uint64_t timestamp_ns;  // 8 bytes
    uint16_t metric_id;     // 2 bytes (compile-time assigned)
    double value;           // 8 bytes
    uint32_t metadata;      // 4 bytes (flags, strategy_id, etc.)
    // Total: 22 bytes per sample
};
```

**Storage Calculation:**
- 500K samples/sec × 22 bytes = 11 MB/sec
- Per hour: 39.6 GB
- Retention: 2 hours = ~80GB (reasonable)

**Files to Create:**
- `include/BinaryWriter.hpp`
- `include/BinaryReader.hpp` (for forensic queries)

---

### Component 3: Aggregation Engine (Tier 2)

**Purpose:** Compute statistics over time windows
**Technology:** C++, runs in background thread

**Statistics Computed:**
- Count, Sum
- Min, Max, Mean
- Standard Deviation
- Percentiles: p50, p95, p99, p999
- Histograms (configurable buckets)

**Aggregation Windows:**
- Configurable: 100ms, 1s, 5s, etc.
- Default: 1 second for HFT use cases

**Algorithm:**
- Sliding window with T-Digest for percentiles
- Incremental computation (no full recalculation)
- Lock-free reads from queue

**Files to Create:**
- `include/Aggregator.hpp`
- `include/Statistics.hpp` (stats algorithms)
- `include/TDigest.hpp` (percentile estimation)

---

### Component 4: Database Writers

#### 4a. QuestDB Writer (Preferred for Tier 2)

**Why QuestDB:**
- 1.4-11M rows/sec ingestion rate
- Native time-series support
- SQL interface
- InfluxDB line protocol support

**Schema:**
```sql
CREATE TABLE metrics_1s (
    timestamp TIMESTAMP,
    metric_name SYMBOL,
    min DOUBLE,
    max DOUBLE,
    avg DOUBLE,
    p50 DOUBLE,
    p95 DOUBLE,
    p99 DOUBLE,
    stddev DOUBLE,
    count LONG
) TIMESTAMP(timestamp) PARTITION BY DAY;
```

**Integration:**
- Use InfluxDB line protocol over TCP
- Batch writes every 1 second
- Non-blocking (queue if unavailable)

**Files to Create:**
- `include/QuestDBWriter.hpp`

#### 4b. Prometheus Exporter (For Tier 3)

**Purpose:** Export to Prometheus for Grafana compatibility
**Protocol:** Prometheus text exposition format

**Features:**
- HTTP endpoint (e.g., :9090/metrics)
- Scrape interval: 15-60 seconds
- Export aggregated stats only (not raw data)

**Example Output:**
```
# TYPE trade_latency_avg gauge
trade_latency_avg 245.3
# TYPE trade_latency_p99 gauge
trade_latency_p99 892.1
# TYPE orders_sent_total counter
orders_sent_total 15234
```

**Files to Create:**
- `include/PrometheusExporter.hpp`
- `examples/prometheus_server.cpp`

---

### Component 5: Visualization Layer

#### 5a. Custom Real-Time Dashboard (for Group 2)

**Technology Stack:**
- **Backend:** C++ WebSocket server (or Rust)
- **Frontend:** React + Recharts/D3.js/Plotly
- **Protocol:** WebSocket for real-time updates

**Features:**
- Sub-second data refresh
- Zoom into microsecond windows
- Multiple metric types (line, histogram, heatmap)
- Alerting on thresholds

**Data Flow:**
```
Aggregator → WebSocket Server → React Frontend
    ↓ every 100ms-1s
```

**Files to Create:**
- `dashboard/backend/ws_server.cpp`
- `dashboard/frontend/` (React app)
- `dashboard/frontend/src/components/TimeSeriesChart.tsx`
- `dashboard/frontend/src/components/HistogramChart.tsx`

#### 5b. Grafana Dashboards (for Group 3)

**Purpose:** Standard dashboards using Prometheus data source
**Features:**
- Pre-built dashboard templates
- JSON dashboard definitions
- Standard panels: latency, throughput, errors

**Files to Create:**
- `grafana/dashboards/hft_overview.json`
- `grafana/dashboards/strategy_performance.json`

---

## Implementation Phases

### Phase 1: Core Library (Weeks 1-3)

**Goal:** Lock-free metrics collection working

**Tasks:**
1. Port QLOG's LockFreeQueue to metrics use case
2. Implement MetricMessage types (counter, gauge, histogram, timer)
3. Create MetricsCollector API
4. Write comprehensive unit tests
5. Benchmark overhead (target: <20ns)

**Deliverables:**
- Header-only C++ library
- Benchmarks showing 10-20ns overhead
- Example usage code

**Success Criteria:**
- ✅ <20ns overhead for record() operation
- ✅ Zero memory allocation on critical path
- ✅ Thread-safe (SPSC and MPSC variants)

---

### Phase 2: Storage Backends (Weeks 4-5)

**Goal:** Data persistence and aggregation

**Tasks:**
1. Implement BinaryWriter with memory-mapped files
2. Build Aggregator with statistics computation
3. Integrate QuestDB writer
4. Add Prometheus exporter
5. File rotation and cleanup logic

**Deliverables:**
- Binary storage working
- QuestDB integration
- Prometheus endpoint

**Success Criteria:**
- ✅ Can store 500K samples/sec to binary files
- ✅ Aggregator computes stats in <10ms per window
- ✅ QuestDB writes 1K aggregates/sec
- ✅ Prometheus scraping works

---

### Phase 3: Visualization (Weeks 6-8)

**Goal:** Real-time and standard dashboards

**Tasks:**
1. Build WebSocket server for real-time data
2. Create React dashboard with charts
3. Design Grafana dashboard templates
4. Add alerting capabilities
5. Polish UI/UX

**Deliverables:**
- Custom React dashboard
- Grafana dashboards
- Documentation

**Success Criteria:**
- ✅ Real-time dashboard updates every 100ms
- ✅ Can zoom into microsecond windows
- ✅ Grafana dashboards load from Prometheus

---

### Phase 4: Production Hardening (Weeks 9-10)

**Goal:** Production-ready

**Tasks:**
1. Error handling and recovery
2. Monitoring and observability (meta-metrics)
3. Performance tuning
4. Documentation
5. Example integrations

**Deliverables:**
- Production deployment guide
- Performance tuning guide
- Integration examples
- Docker containers

**Success Criteria:**
- ✅ Handles queue overflow gracefully
- ✅ Recovers from backend failures
- ✅ <0.1% overhead in production workloads

---

## Repository Structure

```
fast-metrics/
├── README.md
├── LICENSE (Apache 2.0 or MIT)
├── CMakeLists.txt
├── include/
│   ├── fast_metrics/
│   │   ├── core/
│   │   │   ├── LockFreeQueue.hpp
│   │   │   ├── MetricMessage.hpp
│   │   │   ├── MetricsCollector.hpp
│   │   │   └── StringCT.hpp
│   │   ├── storage/
│   │   │   ├── BinaryWriter.hpp
│   │   │   ├── BinaryReader.hpp
│   │   │   └── MemoryMappedFile.hpp
│   │   ├── aggregation/
│   │   │   ├── Aggregator.hpp
│   │   │   ├── Statistics.hpp
│   │   │   └── TDigest.hpp
│   │   ├── exporters/
│   │   │   ├── QuestDBWriter.hpp
│   │   │   ├── PrometheusExporter.hpp
│   │   │   └── InfluxDBWriter.hpp (optional)
│   │   └── utils/
│   │       ├── TimeStamp.hpp
│   │       └── Common.hpp
├── src/
│   ├── dashboard/
│   │   ├── backend/
│   │   │   ├── ws_server.cpp
│   │   │   └── http_server.cpp
│   │   └── frontend/
│   │       ├── package.json
│   │       ├── src/
│   │       │   ├── App.tsx
│   │       │   ├── components/
│   │       │   │   ├── TimeSeriesChart.tsx
│   │       │   │   ├── HistogramChart.tsx
│   │       │   │   └── MetricCard.tsx
│   │       │   └── api/
│   │       │       └── websocket.ts
│   │       └── public/
├── benchmarks/
│   ├── latency_benchmark.cpp
│   ├── throughput_benchmark.cpp
│   └── comparison_vs_otel.cpp
├── examples/
│   ├── basic_usage.cpp
│   ├── hft_trading_simulation.cpp
│   ├── prometheus_integration.cpp
│   └── custom_dashboard_example.cpp
├── tests/
│   ├── unit/
│   │   ├── test_lockfree_queue.cpp
│   │   ├── test_metrics_collector.cpp
│   │   └── test_aggregator.cpp
│   └── integration/
│       ├── test_end_to_end.cpp
│       └── test_prometheus_export.cpp
├── grafana/
│   └── dashboards/
│       ├── hft_overview.json
│       └── strategy_performance.json
├── docker/
│   ├── Dockerfile
│   └── docker-compose.yml (with QuestDB, Prometheus, Grafana)
└── docs/
    ├── architecture.md
    ├── api_reference.md
    ├── performance_tuning.md
    ├── integration_guide.md
    └── benchmarks.md
```

---

## Technology Stack

### Core Library
- **Language:** C++17/20
- **Build System:** CMake 3.15+
- **Testing:** Google Test
- **Benchmarking:** Google Benchmark
- **Style:** Header-only library (easy integration)

### Storage & Databases
- **Tier 1:** Memory-mapped files (mmap)
- **Tier 2:** QuestDB (primary), InfluxDB/TimescaleDB (optional)
- **Tier 3:** Prometheus

### Visualization
- **Backend:** C++ with WebSocket (or Rust for better async)
- **Frontend:** React + TypeScript
- **Charts:** Recharts or D3.js or Plotly.js
- **Grafana:** Version 10+

### Infrastructure
- **CI/CD:** GitHub Actions
- **Containers:** Docker + Docker Compose
- **Documentation:** Doxygen + Markdown

---

## Key Design Decisions

### 1. Header-Only Library
**Decision:** Make core library header-only
**Rationale:**
- Easy integration (no linking)
- Compile-time optimization
- Follows modern C++ best practices (like QLOG)

### 2. Zero Dependencies on Critical Path
**Decision:** No external libraries for metrics collection
**Rationale:**
- Minimize overhead
- No malloc/free
- Predictable performance

### 3. Optional Components
**Decision:** Storage backends and dashboards are optional
**Rationale:**
- Users can integrate with existing infrastructure
- Core library remains lightweight
- Flexibility for different use cases

### 4. Pluggable Exporters
**Decision:** Support multiple backend writers
**Rationale:**
- Different latency groups have different needs
- Users may have existing infrastructure
- Easy to add custom exporters

### 5. Multi-Tier Storage
**Decision:** Store data at multiple granularities
**Rationale:**
- Can't visualize 500K samples/sec in dashboards
- Different time scales for different use cases
- Cost-effective (aggregate old data)

---

## Performance Targets

### Critical Path (Metric Collection)
- **Overhead:** <20ns per metric record
- **Memory:** Zero allocation
- **Throughput:** >1M metrics/sec per thread

### Background Thread (Aggregation)
- **Latency:** <10ms per aggregation window
- **Throughput:** Process 500K samples/sec sustained

### Storage
- **Binary Writer:** >500K samples/sec write
- **QuestDB Writer:** >10K aggregates/sec
- **Prometheus Export:** <100ms scrape time

### Visualization
- **Real-time Dashboard:** <100ms refresh rate
- **Grafana:** Standard (15-60s scrape interval)

---

## Competitive Analysis

| Feature | Fast Metrics | OpenTelemetry | Datadog | Prometheus |
|---------|--------------|---------------|---------|------------|
| **Critical Path Overhead** | 10-20ns | 300-500ns | ~1μs | N/A (pull) |
| **Microsecond Resolution** | ✅ Yes | ❌ No | ❌ No | ❌ No |
| **Lock-Free Collection** | ✅ Yes | ❌ No | ❌ No | ❌ No |
| **Real-Time Dashboards** | ✅ Yes | ❌ No | ✅ Yes | ❌ No |
| **Grafana Compatible** | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes |
| **Open Source** | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes |
| **HFT Optimized** | ✅ Yes | ❌ No | ❌ No | ❌ No |

---

## Go-To-Market Strategy

### Phase 1: Open Source Launch (Month 1-3)
- Release core library as Apache 2.0
- Target: GitHub stars, developer adoption
- Write blog posts about HFT metrics challenges
- Present at QuantCon, High-Frequency Trading conferences

### Phase 2: Community Building (Month 4-6)
- Create examples for common HFT use cases
- Build integrations with popular trading frameworks
- Engage with Group 3 developers (larger market)
- Collect feedback and iterate

### Phase 3: Commercial Features (Month 7-12)
- Offer paid dashboard hosting
- Enterprise support contracts
- Custom integrations for Group 2 firms
- White-label options

### Pricing Tiers
1. **OSS Core** - Free
2. **Pro** - $10K-50K/year (hosted dashboards, support)
3. **Enterprise** - $100K-500K/year (custom deployment, white-glove)

---

## Risk Analysis

### Technical Risks

**Risk 1: Performance Doesn't Meet Targets**
- **Mitigation:** Extensive benchmarking early
- **Fallback:** Market to Group 3 only (less demanding)

**Risk 2: Complex Integration**
- **Mitigation:** Header-only library, minimal dependencies
- **Fallback:** Provide reference implementations

**Risk 3: Visualization Bottleneck**
- **Mitigation:** Pre-aggregate data before sending to frontend
- **Fallback:** Use existing tools (Grafana) more

### Market Risks

**Risk 1: HFT Firms Build In-House**
- **Mitigation:** Make OSS core so compelling they contribute
- **Reality:** They already build in-house, we're offering better

**Risk 2: OpenTelemetry Improves**
- **Mitigation:** Our lock-free architecture is fundamental advantage
- **Reality:** OTel is general-purpose, we're specialized

**Risk 3: Market Too Niche**
- **Mitigation:** Also target Group 3 (10x larger)
- **Reality:** $300M TAM is significant

---

## Success Metrics

### Technical Metrics
- ✅ <20ns overhead demonstrated in benchmarks
- ✅ 500K samples/sec sustained throughput
- ✅ Zero production incidents after 1 month deployment

### Adoption Metrics
- 🎯 100+ GitHub stars in first month
- 🎯 10+ production deployments in 6 months
- 🎯 5+ enterprise customers in 12 months

### Business Metrics
- 🎯 $1M ARR in Year 1
- 🎯 $5M ARR in Year 2
- 🎯 Break-even by Month 18

---

## Next Steps

### Immediate Actions (Week 1)
1. Create new GitHub repository: `fast-metrics`
2. Set up repository structure
3. Port QLOG's LockFreeQueue.hpp as foundation
4. Write basic MetricsCollector API
5. Create first benchmark

### Questions to Answer
1. Should we support C++17 or require C++20?
2. Do we need Windows support or Linux-only initially?
3. What license? (Apache 2.0 recommended for enterprise adoption)
4. Should dashboard be separate repository?

### Resources Needed
- 1-2 C++ engineers (3 months)
- 1 frontend engineer (1 month for dashboard)
- Cloud credits for testing ($1K/month)
- Access to HFT developers for feedback (critical!)

---

## References

### QLOG Framework
- Current repository: `/home/user/qlog`
- Key files to reference:
  - `include/LockFreeQueue.hpp` - Lock-free circular buffer
  - `include/AsyncLogger.hpp` - Message templates
  - `include/StringCT.hpp` - Compile-time strings
  - `loggerbenchmark.cpp` - Benchmark patterns

### Research Links
- [QuestDB Performance Benchmarks](https://questdb.com/blog/timescaledb-vs-questdb-comparison/)
- [OpenTelemetry C++ Performance](https://opentelemetry-cpp.readthedocs.io/en/latest/performance/benchmarks.html)
- [HFT Latency Requirements 2025](https://www.tuvoc.com/blog/low-latency-trading-systems-guide/)
- [Algorithmic Trading Market Size](https://www.fortunebusinessinsights.com/algorithmic-trading-market-107174)

### Target Audience Research
- [Top 100 Quant Firms 2025](https://www.quantblueprint.com/post/top-100-quantitative-trading-firms-to-know-in-2025)
- Software HFT: Citadel Securities, Virtu, Flow Traders, Optiver
- Low-Latency Algo: Two Sigma, DE Shaw, WorldQuant

---

## Appendix: API Examples

### Example 1: Basic Usage
```cpp
#include <fast_metrics/MetricsCollector.hpp>

using namespace fast_metrics;

int main() {
    // Create collector with 64-byte messages, 1MB queue
    MetricsCollector<64, 1024*1024> metrics("output.bin");

    // Start background thread
    metrics.start();

    // Critical path - 10-20ns overhead
    uint64_t start = rdtsc();
    // ... trading logic ...
    uint64_t end = rdtsc();

    metrics.record<"TradeLatency">(end, end - start);
    metrics.counter<"TradesExecuted">()++;

    // Cleanup
    metrics.stop();
    return 0;
}
```

### Example 2: Multiple Metrics
```cpp
// Define metric labels at compile time
using Labels = MetricLabels<
    SCT("OrderLatency"),
    SCT("FillLatency"),
    SCT("BookUpdateLatency"),
    SCT("OrdersSent"),
    SCT("OrdersFilled"),
    SCT("QueueDepth")
>;

MetricsCollector<64, 1024*1024, Labels> metrics;

// Usage
metrics.timer<"OrderLatency">().start();
// ... send order ...
metrics.timer<"OrderLatency">().stop();

metrics.histogram<"FillLatency">().record(latency_ns);
metrics.gauge<"QueueDepth">() = queue.size();
```

### Example 3: With QuestDB
```cpp
#include <fast_metrics/exporters/QuestDBWriter.hpp>

// Configure exporters
auto questdb = QuestDBWriter("localhost", 9009);
auto prometheus = PrometheusExporter(9090);

MetricsCollector metrics;
metrics.addExporter(questdb);
metrics.addExporter(prometheus);

metrics.start();
// Metrics automatically exported every 1s (QuestDB) and on scrape (Prometheus)
```

---

## Contact & Continuation

**To continue this project in a new chat, provide:**
1. This plan document (copy entire markdown)
2. The context: "I want to build a fast metrics framework based on QLOG's lock-free queue architecture for HFT/algorithmic trading"
3. Specify which phase to start with (recommend: Phase 1 - Core Library)

**Repository to create:**
- Name: `fast-metrics` (or `hft-metrics`, `qmetrics`, etc.)
- Location: Separate from QLOG
- License: Apache 2.0 (recommended) or MIT

---

**END OF PLAN**
