#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>

using namespace std;
using namespace std::chrono;
using namespace spdlog;
using namespace spdlog::sinks;

void spdlog_thread_fun(std::shared_ptr<spdlog::logger> logger, int howmany) {
    for (int i = 0; i < howmany; i++) {
        logger->info("Hello logger: msg number x");
        
        // logger->info("Hello logger: msg number {}", 1.23);

        // double floating_number = 3.14;
        // logger->info("Hello logger: msg number {}", floating_number);

        // logger->info("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
    }
}

void spdlog_mt(int howmany, std::shared_ptr<spdlog::logger> logger, int thread_count) {
    using std::chrono::high_resolution_clock;
    std::vector<std::thread> threads;
    auto start = high_resolution_clock::now();

    int msgs_per_thread = howmany / thread_count;
    int msgs_per_thread_mod = howmany % thread_count;
    for (int t = 0; t < thread_count; ++t) {
        if (t == 0 && msgs_per_thread_mod)
            threads.push_back(
                std::thread(spdlog_thread_fun, logger, msgs_per_thread + msgs_per_thread_mod));
        else
            threads.push_back(std::thread(spdlog_thread_fun, logger, msgs_per_thread));
    }

    for (auto &t : threads) {
        t.join();
    };

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << "secs\t" << int(howmany / delta_d) * (70+74) / 1024 /1024  << " MB/sec" << std::endl << std::endl;
}

void spdlog_sync_bench(int howmany, int threads) {
    auto spdlog_sync_logger = spdlog::basic_logger_mt("spdlog_sync_logger", "/Users/jimmyliu/VSCodeProjects/log_analysist_tool/build/logs/spdlog_sync_log.txt", true);
    spdlog_sync_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] <%t>  %v");
    spdlog_mt(howmany, std::move(spdlog_sync_logger), threads);
}

void spdlog_async_bench(int howmany, int threads, int queue_size) {
    spdlog::init_thread_pool(queue_size, 1);    // 设置异步缓存队列大小
    auto spdlog_async_logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "/Users/jimmyliu/VSCodeProjects/log_analysist_tool/build/logs/spdlog_async_log.txt", true);
    spdlog_async_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] <%t>  %v");
    spdlog_mt(howmany, std::move(spdlog_async_logger), threads);
}