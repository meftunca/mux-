#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
      threads_.emplace_back([this] { workerThread(); });
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      stop_ = true;
    }
    condition_.notify_all();
    for (auto& thread : threads_) {
      thread.join();
    }
  }

  void addTask(std::function<void()> task) {
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      tasks_.push(std::move(task));
    }
    condition_.notify_one();
  }

 private:
  void workerThread() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queueMutex_);
        condition_.wait(lock, [this] { return !tasks_.empty() || stop_; });
        if (stop_ && tasks_.empty()) {
          return;
        }
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      task();
    }
  }

  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queueMutex_;
  std::condition_variable condition_;
  bool stop_{};
};
