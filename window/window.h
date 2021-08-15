#pragma once
#include <GLFW/glfw3.h>

#include <string_view>
#include <thread>
#include <atomic>
#include <memory>

#include "core_messages/control.h"
#include "framework/context.h"

namespace pt {

namespace window::detail {
    struct WindowDelete{
        void operator()(GLFWwindow* window);
    };

    struct GlfwInitRaii {
        GlfwInitRaii();
        ~GlfwInitRaii();

        GlfwInitRaii(const GlfwInitRaii&) = delete;
        GlfwInitRaii& operator=(const GlfwInitRaii&) = delete;

        GlfwInitRaii(GlfwInitRaii&&);
        GlfwInitRaii& operator=(GlfwInitRaii&&);

        bool owns;
    };
}

class Window {
public:
    Window(int initial_width, int initial_height, std::string_view title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = default;
    Window& operator=(Window&&) = default;

    EVENT(ProgramStart) {
        poll_thread = std::thread([&ctx, window=window.get(), stop_poll=stop_poll.get()]{
            while (!stop_poll->load()) {
                glfwWaitEvents();
                if (glfwWindowShouldClose(window)) {
                    ctx.emit(QuitRequested{0});
                    return;
                }
            }
        });

        co_return;
    }


private:
    std::unique_ptr<std::atomic<bool>> stop_poll;
    std::thread poll_thread;
    window::detail::GlfwInitRaii glfw_raii;
    std::unique_ptr<GLFWwindow, window::detail::WindowDelete> window;
};

}
