#pragma once

#include <mutex>
#include <deque>
#include <functional>
#include <future>

template <int N>
class simple_task_executor {
    std::mutex mtx;
    std::condition_variable deque_is_changed;
    std::deque<std::packaged_task<void()>> work_deque;

    std::future<void> threads[N];

    std::atomic<bool> stop_flag = false;
    std::atomic<bool> finish_flag = false;

    inline bool execution(const bool wait_for_queue = true) {
        if (stop_flag) return false;

        std::packaged_task<void()> work;
        {
            std::unique_lock<std::mutex> lock(mtx);
            deque_is_changed.wait(lock, [&] {return finish_flag || !wait_for_queue || !work_deque.empty(); });
            if (work_deque.empty()) return false;
            work = std::move(work_deque.front());
            work_deque.pop_front();
        }

        work();
        return true;
    }

    inline void execution_loop() {
        while (execution());
    }

    inline void finish() {
        finish_flag = true;
        deque_is_changed.notify_all();
    }
public:
    ~simple_task_executor() {
        finish();
    }

    simple_task_executor(const simple_task_executor&) = delete;
    simple_task_executor& operator=(const simple_task_executor&) = delete;

    simple_task_executor(simple_task_executor&&) = delete;
    simple_task_executor& operator=(simple_task_executor&&) = delete;

    simple_task_executor() {
        for (std::size_t i = 0; i < N; ++i) {
            threads[i] = std::async(std::launch::async, [this] { execution_loop(); });
        }
    }

    inline std::future<void> queue(std::function<void()>&& task) {
        if (stop_flag || finish_flag) return {};

        std::packaged_task<void()> p_task(std::forward<std::function<void()>>(task));

        auto result_future = p_task.get_future();
        {
            std::lock_guard<std::mutex> lock(mtx);
            work_deque.emplace_back(std::move(p_task));
        }
        deque_is_changed.notify_one();

        return result_future;
    }

    // true - task is finished
    // false - thread pool is stopped
    inline bool wait_for(std::future<void>& task_future) {
        while (!stop_flag) {
            if (task_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                task_future.get();
                return true;
            }

            execution(false);
        }

        return false;
    }

    inline void stop() {
        stop_flag = true;
        deque_is_changed.notify_all();
    }

    inline bool is_stopped() const { return stop_flag; }

    inline void wait_for_stop() {
        for (std::future<void>& thread : threads) {
            thread.get();
        }
    }

    inline void cancel_pending() {
        std::lock_guard<std::mutex> lock(mtx);
        work_deque.clear();
    }
};