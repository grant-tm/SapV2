#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    // Push a value onto the queue, guaranteed to push
    void push(T value);

    // Pop if the queue is not empty, return whether popped or not
    bool try_pop(T& value);

    // Wait and pop a value from the queue
    void wait_pop(T& value);

    // Get the size of the queue
    int size() const;

    // Check if the queue is empty
    bool empty() const;

    // Check if the queue is in producing state
    bool is_producing();

    // Start producing state
    void start_producing();

    // Stop producing state
    void stop_producing();

private:
    mutable std::mutex mutex;
    std::queue<T> queue;
    std::condition_variable cv;
    bool producing = false;
};

// push a value on the queue
// gauranteed to push
template <typename T>
void ThreadSafeQueue<T>::push (T value) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(value));
    cv.notify_one();
}

// pop if the queue is not empty
// return whether popped or not
template <typename T>
bool ThreadSafeQueue<T>::try_pop (T& value) {
    std::unique_lock<std::mutex> lock(mutex);
    if (queue.empty()) {
        return false;
    }
    value = std::move(queue.front());
    queue.pop();
    return true;
}

template <typename T>
void ThreadSafeQueue<T>::wait_pop (T& value) {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this]() { return !queue.empty(); });
    value = std::move(queue.front());
    queue.pop();
}

template <typename T>
int ThreadSafeQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

template <typename T>
bool ThreadSafeQueue<T>::is_producing (void) {
    bool result;
    {
        std::unique_lock<std::mutex> lock(mutex);
        result = producing;
    }
    return result;
}

template <typename T>
void ThreadSafeQueue<T>::start_producing (void) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        producing = true;
    }
    cv.notify_all();
}

template <typename T>
void ThreadSafeQueue<T>::stop_producing (void) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        producing = false;
    }
    cv.notify_all();
}

#endif