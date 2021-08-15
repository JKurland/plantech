#include "framework/context.h"

namespace pt {

namespace context::detail {
    joinable make_joinable(Task<>&& task, std::atomic<int>& running_count, std::coroutine_handle<>& continuation) {
        co_await task;
    }
}

}
