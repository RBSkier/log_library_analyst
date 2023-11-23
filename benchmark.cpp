#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/bundled/format.h"

#include "utils.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <string>

using namespace std;
using namespace std::chrono;
using namespace spdlog;
using namespace spdlog::sinks;
using namespace utils;

void spdlog_bench_mt(int howmany, std::shared_ptr<spdlog::logger> log, int thread_count);

int main(int argc, char *argv[]) {
    int howmany = 1000000;
    int threads = 10;
    int queue_size = std::min(howmany + 2, 819200);

    try {
        if (argc > 1) howmany = atoi(argv[1]);
        if (argc > 2) threads = atoi(argv[2]);
        if (argc > 3) {
            queue_size = atoi(argv[3]);
            if (queue_size > 500000) {
                spdlog::error("Max queue size allowed: 500,000");
                exit(1);
            }
        }

        auto slot_size = sizeof(spdlog::details::async_msg);
        std::cout << "-------------------------------------------------" << std::endl;
        std::cout << "Messages     : " << howmany << std::endl;
        std::cout << "Threads      : " << threads << std::endl;
        std::cout << "Queue        : " << queue_size << " slots" << std::endl;
        std::cout << "Queue memory : " << queue_size << " x " << slot_size << " = " << (queue_size * slot_size) / 1024 / 1024 << " MB " << std::endl;
        std::cout << "-------------------------------------------------" << std::endl;

        std::cout << "*********************************" << std::endl;
        std::cout << "Spdlog Sync Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        auto spdlog_sync_logger = spdlog::basic_logger_mt("spdlog_sync_logger", "logs/spdlog_async_log.txt", true);
        spdlog_sync_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
        spdlog_bench_mt(howmany, std::move(spdlog_sync_logger), threads);

        std::cout << "*********************************" << std::endl;
        std::cout << "Spdlog Async Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        spdlog::init_thread_pool(queue_size, 1);    // 设置异步缓存队列大小
        auto spdlog_async_logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/spdlog_async_log.txt", true);
        spdlog_async_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
        spdlog_bench_mt(howmany, std::move(spdlog_async_logger), threads);
    }
    catch (std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        perror("Last error");
        return 1;
    }

    return 0;
}

void thread_fun(std::shared_ptr<spdlog::logger> logger, int howmany) {
    for (int i = 0; i < howmany; i++) {
        // logger->info("Hello logger: msg number x");

        // logger->info("Hello logger: msg number {}", i);

        // double floating_number = 3.14;
        // logger->info("Hello logger: msg number {}", floating_number);

        logger->info("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
    }
}

void spdlog_bench_mt(int howmany, std::shared_ptr<spdlog::logger> logger, int thread_count) {
    using std::chrono::high_resolution_clock;
    vector<std::thread> threads;
    auto start = high_resolution_clock::now();

    int msgs_per_thread = howmany / thread_count;
    int msgs_per_thread_mod = howmany % thread_count;
    for (int t = 0; t < thread_count; ++t) {
        if (t == 0 && msgs_per_thread_mod)
            threads.push_back(
                std::thread(thread_fun, logger, msgs_per_thread + msgs_per_thread_mod));
        else
            threads.push_back(std::thread(thread_fun, logger, msgs_per_thread));
    }

    for (auto &t : threads) {
        t.join();
    };

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << "secs\t" << int(howmany / delta_d) * 100 / 1024 / 1024 << " MB/sec" << std::endl << std::endl;
}