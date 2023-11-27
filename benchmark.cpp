#include "spdlog_bench.h"
#include "glog_bench.h"
#include "boost_sync_bench.h"
#include "boost_async_bench.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

using namespace std;
using namespace std::chrono;

int main(int argc, char *argv[]) {
    int howmany = 1000000;
    int thread_count = 10;
    int queue_size = howmany + 2;

    try {
        if (argc > 1) howmany = atoi(argv[1]);
        if (argc > 2) thread_count = atoi(argv[2]);
        if (argc > 3) {
            queue_size = atoi(argv[3]);
            if (queue_size > 500000) {
                std::cout << "Max queue size allowed: 500,000" << std::endl;
                exit(1);
            }
        }

        auto slot_size = sizeof(spdlog::details::async_msg);
        std::cout << "-------------------------------------------------" << std::endl;
        std::cout << "Messages     : " << howmany << std::endl;
        std::cout << "Threads      : " << thread_count << std::endl;
        std::cout << "Queue        : " << queue_size << " slots" << std::endl;
        std::cout << "Queue memory : " << queue_size << " x " << slot_size << " = " << (queue_size * slot_size) / 1024 / 1024 << " MB " << std::endl;
        std::cout << "-------------------------------------------------" << std::endl;

        std::cout << "*********************************" << std::endl;
        std::cout << "Spdlog: Sync Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        spdlog_sync_bench(howmany, thread_count);

        std::cout << "*********************************" << std::endl;
        std::cout << "Spdlog: Async Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        spdlog_async_bench(howmany, thread_count, queue_size);

        std::cout << "*********************************" << std::endl;
        std::cout << "Glog: Sync Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        glog_sync_bench(howmany, thread_count);

        std::cout << "*********************************" << std::endl;
        std::cout << "Boost Log: Sync Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        boost_sync_test(howmany, thread_count, queue_size);

        std::cout << "*********************************" << std::endl;
        std::cout << "Boost Log: Async Log" << std::endl;
        std::cout << "*********************************" << std::endl;

        boost_async_test(howmany, thread_count, queue_size);
    }
    catch (std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        perror("Last error");
        return 1;
    }

    return 0;
}