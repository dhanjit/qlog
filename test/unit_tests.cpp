#include <gtest/gtest.h>
#include <cstring>
#include <thread>
#include <vector>
#include "AsyncLogger.hpp"
#include "StringCT.hpp"

TEST(StringCTTest, BasicConstruction) {
    using TestStr = common::stringct::StringCT<'T', 'e', 's', 't'>;
    ASSERT_STREQ(TestStr::str, "Test");
}

#include <thread>
#include <vector>
#include "LockFreeQueue.hpp"

struct TestMsg {
    int id;
    char data[32];
};

TEST(LockFreeQueueTest, FixedMessageLFQ_Basic) {
    common::logger::FixedMessageLFQ<64, 1024> queue;
    TestMsg msg{1, "Hello World"};

    // Push
    queue.push(msg);

    // Pop
    ASSERT_FALSE(queue.empty());
    const void* popPtr = queue.front();
    ASSERT_NE(popPtr, nullptr);
    const TestMsg* received = static_cast<const TestMsg*>(popPtr);
    ASSERT_EQ(received->id, 1);
    ASSERT_STREQ(received->data, "Hello World");
    queue.pop();
    ASSERT_TRUE(queue.empty());
}

TEST(LockFreeQueueTest, FixedMessageLFQ_SPSC) {
    common::logger::FixedMessageLFQ<64, 1024> queue;
    const int iterations = 10000;
    std::atomic<bool> done{false};

    std::thread producer([&]() {
        for (int i = 0; i < iterations; ++i) {
            while (!queue.canEnqueue(sizeof(TestMsg))) {
                std::this_thread::yield();
            }
            TestMsg msg{i, "Data"};
            queue.push(msg);
        }
        done = true;
    });

    std::thread consumer([&]() {
        int expected = 0;
        while (!done || !queue.empty()) {
            if (!queue.empty()) {
                const void* ptr = queue.front();
                ASSERT_NE(ptr, nullptr);
                const TestMsg* msg = static_cast<const TestMsg*>(ptr);
                ASSERT_EQ(msg->id, expected++);
                queue.pop();
            } else {
                std::this_thread::yield();
            }
        }
        ASSERT_EQ(expected, iterations);
    });

    producer.join();
    consumer.join();
}

#include <fstream>
#include <string>
#include "SpscAsyncLogger.hpp"

TEST(AsyncLoggerTest, SpscLogger_Basic) {
    const std::string filename = "test_log.txt";
    // Remove existing file
    std::remove(filename.c_str());

    {
        common::logger::SpscAsyncLogger<64, 1024> logger(filename + ".bak", filename, 1u);
        logger.start("TestLogger");

        using LL = common::logger::label::LabelList<common::logger::level::INFO>;
        logger.log<LL>(common::timestamp::MicroSecondTime{}, 1, 2, 3);

        // Ensure flush happens on destruction or manually
        // logger.stop() is called in destructor
    }

    // Verify file content
    std::ifstream ifs(filename);
    ASSERT_TRUE(ifs.good());
    std::string line;
    // First line is init info
    std::getline(ifs, line);
    ASSERT_NE(line.find("LoggerInit"), std::string::npos);

    // Second line is our log
    std::getline(ifs, line);
    // Expected format depends on LabelList and args.
    // LL has INFO. Args are 1, 2, 3.
    // Format: Timestamp, [INFO], 1, 2, 3
    // Timestamp format depends on TimeStamp implementation.
    // LabelList uses SCT("INF") -> "INF".
    // Delimiter is ','.
    ASSERT_NE(line.find("INF"), std::string::npos);
    ASSERT_NE(line.find("1,2,3"), std::string::npos);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
