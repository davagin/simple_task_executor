// main.cpp

#include <gtest/gtest.h>
#include <iostream>

#include "simple_task_executor.h"

TEST(simple_thread_pool, all_threads_finished) {
    constexpr int NUMBER_OF_THREADS = 10;
    constexpr int NUMBER_OF_TASKS = 20;

    simple_task_executor<NUMBER_OF_THREADS> task_executor;
    std::atomic<int> counter = 0;
    std::future<void> results[NUMBER_OF_TASKS];
    for (int i = 0; i < NUMBER_OF_TASKS; i++) {
        results[i] = task_executor.queue(std::function<void()>([i, &counter]() {
            std::cout << "Task #" << i << std::endl;
            counter++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }));
    }

    for (std::future<void>& result_future : results)
        result_future.get();

    std::cout << "Counter: " << counter << std::endl;
    ASSERT_EQ(counter, NUMBER_OF_TASKS);
}

TEST(simple_thread_pool, all_threads_stopped) {
    constexpr int NUMBER_OF_THREADS = 10;
    constexpr int NUMBER_OF_TASKS = 20;

    simple_task_executor<NUMBER_OF_THREADS> task_executor;
    std::atomic<int> counter = 0;
    std::future<void> results[NUMBER_OF_TASKS];
    for (int i = 0; i < NUMBER_OF_TASKS; i++) {
        results[i] = task_executor.queue(std::function<void()>([i, &counter]() {
            std::cout << "Task #" << i << std::endl;
            counter++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }));
    }

    task_executor.stop();
    task_executor.wait_for_stop();

    std::cout << "Counter: " << counter << std::endl;
    ASSERT_TRUE(counter < NUMBER_OF_TASKS);
}

TEST(simple_thread_pool, test_recursion) {
    constexpr int NUMBER_OF_THREADS = 2;

    simple_task_executor<NUMBER_OF_THREADS> task_executor;

    std::future<void> r0 = task_executor.queue(std::function<void()>([&task_executor]() {
        std::cout << "Task #0" << std::endl;

        std::future<void> r1 = task_executor.queue(std::function<void()>([&task_executor]() {
            std::cout << "Task #1" << std::endl;

            std::future<void> r2 = task_executor.queue(std::function<void()>([&task_executor]() {
                std::cout << "Task #2" << std::endl;

                std::future<void> r3 = task_executor.queue(std::function<void()>([&task_executor]() {
                    std::cout << "Task #3" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout << "Task #3 finished" << std::endl;
                }));

                task_executor.wait_for(r3);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "Task #2 finished" << std::endl;
            }));

            task_executor.wait_for(r2);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Task #1 finished" << std::endl;
        }));

        task_executor.wait_for(r1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Task #0 finished" << std::endl;
    }));

    task_executor.wait_for(r0);

    ASSERT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
