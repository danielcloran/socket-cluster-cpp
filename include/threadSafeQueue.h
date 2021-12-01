#ifndef ThreadSafeQueue_h
#define ThreadSafeQueue_h

#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

template <class T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue(int maxSize = 200) : maxSize(maxSize), q(), m(), c() {}
    ~ThreadSafeQueue() {}

    // Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        if (q.size() < maxSize) {
            q.push_back(t);
        }
        else {
            /*
                If this error is thrown, the write rate is too high, or the socket
                has disconnected without recognizing and the queue is backing up.
            */
            auto currentClock = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(currentClock);
            std::cerr << std::ctime(&currentTime) << "Message Queue Full. Dumping oldest message." << std::endl;
            q.pop_back();
            q.push_back(t);
        }
        // std::cout<< "Adding to msg Queue size is now: " << q.size() << std::endl;
        c.notify_one();
    }

    void push_front(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        if (q.size() >= maxSize)
        {
            /*
                If this error is thrown, the write rate is too high, or the socket
                has disconnected without recognizing and the queue is backing up.
            */
            auto currentClock = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(currentClock);
            std::cerr << std::ctime(&currentTime) << "Push_front exceeds max queue size: allowing anyway." << std::endl;\
        }
        q.push_front(t);
        c.notify_one();
    }


    // Get the front element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) return "";
        T val = q.front();
        q.pop_front();
        // std::cout<< "Removing from msg Queue size is now: " << q.size() << std::endl;
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

    void clear(void)
    {
        std::unique_lock<std::mutex> lock(m);
        q.clear();
    }

private:
    std::deque<T> q;
    mutable std::mutex m;
    std::condition_variable c;
    int maxSize;
};

#endif // ThreadSafeQueue_h
