#pragma once
namespace pt {

// allocates an object on the heap, raii manages it's memory and gives
// comparison and copy operations when possible. Unlike unique_ptr Box 
// cannot be null.
template<typename T>
class Box {
public:
    Box(T t): t(new T(std::move(t))) {}

    template<typename...Ts>
    static Box emplace(Ts&&...ts) {
        return Box(new T(std::forward<Ts>(ts)...));
    }

    ~Box() {delete t;}

    Box(const Box& o): t(new T(*o.t)) {}
    Box& operator=(const Box& o) {
        if (&o == this) return *this;

        *t = *o.t;
        return *this;
    }

    Box(Box&& o): t(new T(std::move(*o.t))) {
        o.t = nullptr;
    }
    
    Box& operator=(Box&& o) {
        if (&o == this) return *this;

        *t = std::move(*o.t);
        o.t = nullptr;
        return *this;
    }

    auto operator<=>(const Box& o) const {
        return *t <=> *o.t;
    }

    bool operator==(const Box& o) const {
        return *t == *o.t;
    }

    T& operator*() {return *t;}
    const T& operator*() const {return *t;}

    T* operator->() {return t;}
    const T* operator->() const {return t;}
private:
    Box(T* t): t(t) {}

    T* t;
};


}
