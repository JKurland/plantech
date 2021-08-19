#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <string_view>
#include <thread>
#include <atomic>
#include <memory>
#include <type_traits>
#include <functional>
#include <iostream>

#include "core_messages/control.h"
#include "framework/context.h"

namespace pt {

struct KeyPress {
    int glfw_key;
};

struct NewWindow {
    GLFWwindow* window;
};

struct WindowResize {
    int width;
    int height;
};

struct WindowMinimised {};
struct WindowRestored {};
struct ClosingWindow {
    GLFWwindow* window;
};

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

    // callbacks is the GLFWwindow user pointer
    struct Callbacks {
        std::function<std::remove_pointer_t<GLFWkeyfun>> key_cb;
        std::function<std::remove_pointer_t<GLFWframebuffersizefun>> resize_cb;

        // does minimise and restore
        std::function<std::remove_pointer_t<GLFWwindowiconifyfun>> iconify_cb;
    };

    // free functions forward the glfw callback to the callback in Callbacks
    void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
    void resize_cb(GLFWwindow* window, int width, int height);
    void iconify_cb(GLFWwindow* window, int iconified);
}

class Window {
public:
    Window(int initial_width, int initial_height, std::string_view title);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = default;
    Window& operator=(Window&&) = default;

    EVENT(ProgramStart) {
        ctx.emit(NewWindow{window.get()});

        callbacks->key_cb = [&ctx](GLFWwindow* window, int key, int scancode, int action, int mods) {
            ctx.emit(KeyPress{key});
        };
    
        callbacks->resize_cb = [&ctx](GLFWwindow* window, int width, int height) {
            ctx.emit(WindowResize{width, height});
        };

        callbacks->iconify_cb = [&ctx](GLFWwindow* window, int iconified) {
            if (iconified == GL_TRUE) {
                ctx.emit(WindowMinimised{});
            } else if (iconified == GL_FALSE) {
                ctx.emit(WindowRestored{});
            } else {
                assert(false);
            }
        };

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

    EVENT(KeyPress) {
        if (event.glfw_key == GLFW_KEY_ESCAPE) {
            ctx.emit(QuitRequested{0});
        }
        co_return;
    }

    EVENT(ProgramEnd) {
        stop_poll_thread();
        co_await ctx.emit_await(ClosingWindow{window.get()});
        window.reset();
    }
private:

    void stop_poll_thread();

    std::unique_ptr<window::detail::Callbacks> callbacks;
    std::unique_ptr<std::atomic<bool>> stop_poll;
    std::thread poll_thread;
    window::detail::GlfwInitRaii glfw_raii;
    std::unique_ptr<GLFWwindow, window::detail::WindowDelete> window;
};

}