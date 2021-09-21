#ifndef THREAD_SAFE_QUEUE
#define THREAD_SAFE_QUEUE

#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

template <class T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue(int maxSize = 20) : maxSize(maxSize) {}
    ~ThreadSafeQueue() {}

    // Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        if (q.size() < maxSize) {
            q.push(t);
        }
        else {
            std::cerr << "Message Queue Full. Dumping oldest message." << std::endl;
            q.pop();
            q.push(t);
        }
        c.notify_one();
    }

    // Get the front element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) return "";
        T val = q.front();
        q.pop();
        return val;
    }

    int block_until_value(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        return true;
    }

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
    int maxSize;
};
#endif // THREAD_SAFE_QUEUE