#include <glog/logging.h>

void glog_thread_fun(int howmany) {
    for (int i = 0; i < howmany; i++) {
        LOG(INFO) << "Hello logger: msg number x";
        // LOG(INFO) << "Hello logger: msg number " << 1.23;
        // LOG(INFO) << "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
    }
}

void glog_mt(int howmany, int thread_count) {
    using std::chrono::high_resolution_clock;
    std::vector<std::thread> threads;
    auto start = high_resolution_clock::now();

    int msgs_per_thread = howmany / thread_count;
    int msgs_per_thread_mod = howmany % thread_count;
    for (int t = 0; t < thread_count; ++t) {
        if (t == 0 && msgs_per_thread_mod)
            threads.push_back(
                std::thread(glog_thread_fun, msgs_per_thread + msgs_per_thread_mod));
        else
            threads.push_back(std::thread(glog_thread_fun, msgs_per_thread));
    }

    for (auto &t : threads) {
        t.join();
    };

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << "secs\t" << int(howmany / delta_d) * (79+74) / 1024 / 1024 << " MB/sec" << std::endl << std::endl;
}

void glog_sync_bench(int howmany, int thread_count) {
    FLAGS_log_dir = "/Users/jimmyliu/VSCodeProjects/log_analysist_tool/build/logs";
    google::InitGoogleLogging("glog_bench.h");
    glog_mt(howmany, thread_count);
    google::ShutdownGoogleLogging();
}