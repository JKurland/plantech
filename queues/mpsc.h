#pragma once

#include <vector>
#include <utility>
#include <mutex>
#include <cstdlib>
#include <condition_variable>
#include <bit>
#include <chrono>
#include <optional>

namespace pt {

template<typename T>
class MpscQueue {
public:
    MpscQueue() = default;
    ~MpscQueue();

    MpscQueue(const MpscQueue& o);
    MpscQueue(MpscQueue&& o);

    MpscQueue& operator=(const MpscQueue& o);
    MpscQueue& operator=(MpscQueue&& o);

    void push(T t);
    T pop();
    std::optional<T> wait_until(std::chrono::steady_clock::time_point until);

    bool empty() const;
    void reserve(size_t new_capacity);
private:
    size_t capacity() const;

    // new_capacity must be a power of 2
    void reallocate(size_t new_capacity);

    T& get(size_t i);
    const T& get(size_t i) const;

    T* buffer = nullptr;
    size_t front = 0;
    size_t size = 0;
    size_t capacity_mask = 0;
    std::condition_variable cv;
    mutable std::mutex m;
};


template<typename T>
MpscQueue<T>::~MpscQueue() {
    if (buffer) {
        for (size_t i = 0; i < size; i++) {
            get(i).~T();
        }
        std::free(buffer);
    }
}

template<typename T>
T& MpscQueue<T>::get(size_t i) {
    return *std::launder(&buffer[(i + front) & capacity_mask]);
}

template<typename T>
const T& MpscQueue<T>::get(size_t i) const {
    return *std::launder(&buffer[(i + front) & capacity_mask]);
}

template<typename T>
bool MpscQueue<T>::empty() const {
    std::lock_guard<std::mutex> l(m);
    return size == 0;
}

template<typename T>
void MpscQueue<T>::reserve(size_t new_capacity) {
    if (new_capacity < capacity()) return;
    reallocate(std::bit_ceil(new_capacity));
}

template<typename T>
void MpscQueue<T>::push(T t) {
    {
        std::lock_guard<std::mutex> l(m);

        if (size == capacity()) {
            size_t new_capacity_mask = (capacity_mask << 1) | 1;
            reallocate(new_capacity_mask + 1);
        }

        new (&buffer[(front + size) & capacity_mask]) T(std::move(t));
        size++;
    }
    cv.notify_one();
}

template<typename T>
T MpscQueue<T>::pop() {
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [this]{return size != 0;});
    T ret = std::move(get(0));
    get(0).~T();
    front = (front + 1) & capacity_mask;
    size--;
    return ret;
}

template<typename T>
std::optional<T> MpscQueue<T>::wait_until(std::chrono::steady_clock::time_point until) {
    std::unique_lock<std::mutex> l(m);
    if (cv.wait_until(l, until, [this]{return size != 0;})) {
        auto ret = std::optional<T>(std::move(get(0)));
        get(0).~T();
        front = (front + 1) & capacity_mask;
        size--;
        return ret;
    }
    return std::nullopt;

}

template<typename T>
size_t MpscQueue<T>::capacity() const {
    return capacity_mask ? capacity_mask + 1 : 0;
}

template<typename T>
void MpscQueue<T>::reallocate(size_t new_capacity) {
    if (size == 0) {
        buffer = (T*) std::malloc(new_capacity * sizeof(T));
        capacity_mask = new_capacity - 1;
        front = 0;
        return;
    }

    T* new_buffer = (T*) std::malloc(new_capacity * sizeof(T));
    for (size_t i = 0; i < size; i++) {
        try {
            // can only move if noexcept, since otherwise we would have a partially moved out
            // of thing in the old buffer.
            new (&new_buffer[i]) T(std::move_if_noexcept(buffer[(front + i) & capacity_mask]));
        } catch (...) {
            for (size_t k = 0; k < i; k++) {
                std::launder(&new_buffer[k])->~T();
            }
            std::free(new_buffer);
            throw;
        }
    }

    for (size_t i = 0; i < size; i++) {
        buffer[(front + i) & capacity_mask].~T();
    }
    std::free(buffer);

    buffer = new_buffer;
    front = 0;
    capacity_mask = new_capacity - 1;
}


template<typename T>
MpscQueue<T>::MpscQueue(const MpscQueue& o) {
    reserve(o.size);
    size = o.size;

    for (size_t i = 0; i < o.size; i++) {
        new (&buffer[(front + i) & capacity_mask]) T(o.buffer[(o.front + i) & capacity_mask]);
    }
}

template<typename T>
MpscQueue<T>::MpscQueue(MpscQueue&& o):
    buffer(o.buffer),
    front(o.front),
    size(o.size),
    capacity_mask(o.capacity_mask)
{
    o.front = 0;
    o.size = 0;
    o.capacity_mask = 0;
    o.buffer = nullptr;
}

template<typename T>
MpscQueue<T>& MpscQueue<T>::operator=(const MpscQueue& o) {
    if (this == &o) return *this;

    {
        std::lock_guard l{m};
        for (size_t i = 0; i < size; i++) {
            get(i).~T();
        }
    }

    // need to lock both this and o, need to make sure there's a consistent hierarchy to avoid
    // potential dealocks, use the ordering of the this pointers.
    std::unique_lock<std::mutex> l;
    std::unique_lock<std::mutex> l2;
    if (this < &o) {
        l = std::unique_lock(m);
        l2 = std::unique_lock(o.m);
    } else {
        l = std::unique_lock(o.m);
        l2 = std::unique_lock(m);
    }


    size = 0;
    front = 0;

    reserve(o.size);
    for (size_t i = 0; i < o.size; i++) {
        new (&buffer[(i + front) & capacity_mask]) T(static_cast<const T&>(o.get(i)));
        size++;
    }

    if (size > 0) {
        l2.unlock();
        l.unlock();
        cv.notify_one();
    }

    return *this;
}

template<typename T>
MpscQueue<T>& MpscQueue<T>::operator=(MpscQueue&& o) {
    if (this == &o) return *this;

    std::unique_lock l(m);

    buffer = o.buffer;
    size = o.size;
    capacity_mask = o.capacity_mask;
    front = o.front;

    o.buffer = nullptr;
    o.size = 0;
    o.capacity_mask = 0;
    o.front = 0;

    if (size > 0) {
        l.unlock();
        cv.notify_one();
    }

    return *this;
}


}
