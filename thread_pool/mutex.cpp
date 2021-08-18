#include "thread_pool/mutex.h"

namespace pt {

MutexGuard::~MutexGuard() {
    if (!m->waiting.empty()) {
        m->pool->push(m->waiting.front());
        m->waiting.pop_front();
    } else {
        m->active = false;
    }
}

bool SingleThreadedMutex::awaiter::await_ready() {
    bool was_active = m->active;
    m->active = true;
    return !was_active;
}

MutexGuard SingleThreadedMutex::awaiter::await_resume() {
    return MutexGuard{m};
}

}