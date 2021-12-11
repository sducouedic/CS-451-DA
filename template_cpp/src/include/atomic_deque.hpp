#pragma once

#include <deque>
#include <mutex>

template <class T>
class AtomicDeque
{
public:
    void push(T elem)
    {
        m.lock();
        deque.push_back(elem);
        m.unlock();
    }

    bool has_next()
    {
        return deque.size() != 0;
    }

    T get_next()
    {
        T next = deque.front();
        deque.pop_front();
        return next;
    }

private:
    std::deque<T> deque;
    std::mutex m;
};
