/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   30.08.2009
 *
 * \brief  An example of asynchronous logging in multiple threads.
 */

// #define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
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
#include <boost/log/utility/record_ordering.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using namespace std::chrono;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(async_test_lg, src::logger_mt)

void async_bench_mt(int howmany, int thread_count);

int boost_async_test(int howmany, int threads_count, int queue_size)
{
    try {
        // Open a rotating text file
        boost::shared_ptr< std::ostream > strm(new std::ofstream("logs/boost_asyn.txt"));
        if (!strm->good())
            throw std::runtime_error("Failed to open a text log file");

        // Create a text file sink
        typedef boost::log::sinks::text_ostream_backend backend_t;
        typedef boost::log::sinks::asynchronous_sink<
            backend_t,
            boost::log::sinks::unbounded_ordering_queue<
                logging::attribute_value_ordering< unsigned int, std::less< unsigned int > >
            >
        > sink_t;
        boost::shared_ptr< sink_t > sink(new sink_t(
            boost::make_shared< backend_t >(),
            // We'll apply record ordering to ensure that records from different threads go sequentially in the file
            keywords::order = logging::make_attr_ordering< unsigned int >("RecordID", std::less< unsigned int >())));

        sink->locked_backend()->add_stream(strm);

        sink->set_formatter
        (
            expr::format("%1%: [%2%] [%3%] - %4%")
                % expr::attr< unsigned int >("RecordID")
                % expr::attr< boost::posix_time::ptime >("TimeStamp")
                % expr::attr< boost::thread::id >("ThreadID")
                % expr::smessage
        );

        // Add it to the core
        logging::core::get()->add_sink(sink);

        // Add some attributes too
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());

        async_bench_mt(howmany, threads_count);

        // Flush all buffered records
        sink->stop();
        sink->flush();

        return 0;
    }
    catch (std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        perror("Last error");
        return 1;
    }

    return 0;
}

//! This function is executed in multiple threads
void async_thread_fun(boost::barrier& bar, int howmany)
{
    // Wait until all threads are created
    bar.wait();

    // Here we go. First, identify the thread.
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());

    // Now, do some logging
    for (int i = 0; i < howmany; ++i)
    {
        BOOST_LOG(async_test_lg::get()) << "Hello logger: msg number x";
        // BOOST_LOG(async_test_lg::get()) << "Hello logger: msg number " << i;
    }
}

void async_bench_mt(int howmany, int thread_count) {
    using std::chrono::high_resolution_clock;
    auto start = high_resolution_clock::now();

    // Create logging threads
    boost::barrier bar(thread_count);
    boost::thread_group threads;
    int msgs_per_thread = howmany / thread_count;
    int msgs_per_thread_mod = howmany % thread_count;
    for (int t = 0; t < thread_count; ++t)
        if (t == 0 && msgs_per_thread_mod)
            threads.create_thread(boost::bind(&async_thread_fun, boost::ref(bar), msgs_per_thread + msgs_per_thread_mod));
        else    
            threads.create_thread(boost::bind(&async_thread_fun, boost::ref(bar), msgs_per_thread));

    // Wait until all action ends
    threads.join_all();

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << "secs\t" << int(howmany / delta_d) * 81 / 1024 / 1024 << " MB/sec" << std::endl << std::endl;
}
