/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   10.06.2008
 *
 * \brief  An example of logging in multiple threads.
 *         See the library tutorial for expanded comments on this code.
 *         It may also be worthwhile reading the Wiki requirements page:
 *         http://www.crystalclearsoftware.com/cgi-bin/boost_wiki/wiki.pl?Boost.Logging
 */

// #define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/core/ref.hpp>
#include <boost/bind/bind.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
// namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using namespace std::chrono;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(test_lg, src::logger_mt)

void sync_bench_mt(int howmany, int thread_count);

int boost_sync_test(int howmany, int threads_count, int queue_size)
{
    try
    {
        // Open a rotating text file
        boost::shared_ptr< std::ostream > strm(new std::ofstream("test.log"));
        if (!strm->good())
            throw std::runtime_error("Failed to open a text log file");

        // Create a text file sink
        boost::shared_ptr< boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > > sink(
            new boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend >);    // 创建同步且后端输出为文本的sink，然后返回共享指针

        sink->locked_backend()->add_stream(strm);   // sink backend添加目标流

        sink->set_formatter
        (
            expr::format("%1%: [%2%] [%3%] - %4%")
                % expr::attr< unsigned int >("RecordID")
                % expr::attr< boost::posix_time::ptime >("TimeStamp")
                % expr::attr< attrs::current_thread_id::value_type >("ThreadID")
                % expr::smessage
        );

        // Add it to the core
        logging::core::get()->add_sink(sink);   // 将创建的sink绑定core

        // Add some attributes too
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock()); // 在core层绑定时间戳
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >()); // 绑定每条记录的ID
        logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id()); // 绑定当前线程的ID

        sync_bench_mt(howmany, threads_count);

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}

//! This function is executed in multiple threads
void sync_thread_fun(boost::barrier& bar, int howmany)
{
    // Wait until all threads are created
    bar.wait();

    // Now, do some logging
    for (int i = 0; i < howmany; ++i)
    {
        BOOST_LOG(test_lg::get()) << "Hello logger: msg number x";
        // BOOST_LOG(async_test_lg::get()) << "Hello logger: msg number " << i;
    }
}

void sync_bench_mt(int howmany, int thread_count) {
    using std::chrono::high_resolution_clock;
    auto start = high_resolution_clock::now();

    // Create logging threads
    boost::barrier bar(thread_count);
    boost::thread_group threads;
    int msgs_per_thread = howmany / thread_count;
    int msgs_per_thread_mod = howmany % thread_count;
    for (int t = 0; t < thread_count; ++t)
        if (t == 0 && msgs_per_thread_mod)
            threads.create_thread(boost::bind(&sync_thread_fun, boost::ref(bar), msgs_per_thread + msgs_per_thread_mod));
        else    
            threads.create_thread(boost::bind(&sync_thread_fun, boost::ref(bar), msgs_per_thread));

    // Wait until all action ends
    threads.join_all();

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << "secs\t" << int(howmany / delta_d) * 81 / 1024 /1024  << " MB/sec" << std::endl << std::endl;
}