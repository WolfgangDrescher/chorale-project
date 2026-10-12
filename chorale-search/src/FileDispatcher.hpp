#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace choralesearch {

// Hands the files of a corpus out to worker threads, one per hardware thread, and puts what they
// come back with into file order. Two steps are given to it:
//
//   work     runs for several files at the same time, each on a worker thread, and returns what
//            is to be handed on for the file
//   collect  runs for one file at a time and in file order, with what `work` returned, so it can
//            keep running totals, apply a limit, report progress or call into code that is not
//            written for several threads
//
// The outcome is that of going through the files one by one. `collect` answers whether the next
// file is wanted: once it says no, no further file is started or collected. The first exception
// thrown by either step stops the rest and is thrown from run().
template <typename WorkResult>
class FileDispatcher {
public:
    using Work = std::function<WorkResult(std::size_t fileIndex)>;
    using Collect = std::function<bool(std::size_t fileIndex, WorkResult& result)>;

    FileDispatcher(std::size_t fileCount, Work work, Collect collect)
        : m_fileCount(fileCount), m_work(std::move(work)), m_collect(std::move(collect)) {}

    void run() {
        const std::size_t threads =
            std::max<std::size_t>(1, std::min<std::size_t>(std::thread::hardware_concurrency(), m_fileCount));
        // How far ahead of the file to be collected next the others may get, which is what limits
        // the number of finished files that wait for their turn.
        const std::size_t lookAhead = 2 * threads;

        std::atomic<std::size_t> nextFile{0};
        std::mutex mutex; // guards everything below
        std::condition_variable somethingChanged;
        std::vector<std::optional<WorkResult>> waiting(m_fileCount);
        std::size_t nextToCollect = 0;
        bool stopped = false;
        std::exception_ptr error;

        auto worker = [&] {
            for (;;) {
                const std::size_t index = nextFile.fetch_add(1);
                if (index >= m_fileCount) return;
                try {
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        somethingChanged.wait(lock, [&] { return stopped || index < nextToCollect + lookAhead; });
                        if (stopped) return;
                    }

                    WorkResult result = m_work(index);

                    std::lock_guard<std::mutex> lock(mutex);
                    if (stopped) return;
                    waiting[index] = std::move(result);
                    while (!stopped && nextToCollect < m_fileCount && waiting[nextToCollect]) {
                        const std::size_t collecting = nextToCollect++;
                        const bool wanted = m_collect(collecting, *waiting[collecting]);
                        waiting[collecting].reset();
                        if (!wanted) stopped = true;
                    }
                    somethingChanged.notify_all();
                } catch (...) {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (!error) error = std::current_exception();
                    stopped = true;
                    somethingChanged.notify_all();
                    return;
                }
            }
        };

        std::vector<std::thread> pool;
        for (std::size_t i = 1; i < threads; ++i) pool.emplace_back(worker);
        worker();
        for (std::thread& thread : pool) thread.join();
        if (error) std::rethrow_exception(error);
    }

private:
    std::size_t m_fileCount;
    Work m_work;
    Collect m_collect;
};

} // namespace choralesearch
