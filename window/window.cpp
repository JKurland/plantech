#include "window/window.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <iostream>
#include <atomic>
#include <cstdio>

namespace pt {

Window::Window(int initial_width, int initial_height, std::string_view title, std::function<void()>& pollWindow):
    poll_window(&pollWindow)
{
    callbacks = std::make_unique<window::detail::Callbacks>();
    stop_poll = std::make_unique<std::atomic<bool>>(false);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window.reset(glfwCreateWindow(initial_width, initial_height, title.data(), nullptr, nullptr));
    assert(window);
    glfwSetWindowUserPointer(window.get(), callbacks.get());

    glfwSetKeyCallback(window.get(), window::detail::key_cb);
    glfwSetFramebufferSizeCallback(window.get(), window::detail::resize_cb);
    glfwSetWindowIconifyCallback(window.get(), window::detail::iconify_cb);
    glfwSetMouseButtonCallback(window.get(), window::detail::mouse_button_cb);
}

void Window::stop_poll_thread() {
    *stop_poll = true;
    glfwPostEmptyEvent();
}

namespace window::detail {
    void WindowDelete::operator()(GLFWwindow* window){
        glfwDestroyWindow(window);
    }

    GlfwInitRaii::GlfwInitRaii() {
        assert(glfwInit());
    }
    GlfwInitRaii::~GlfwInitRaii() {
        glfwTerminate();
    }

    static GlfwInitRaii glfwInitRaii;

    void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks->key_cb) {
            callbacks->key_cb(window, key, scancode, action, mods);
        }
    }

    void resize_cb(GLFWwindow* window, int width, int height) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks->resize_cb) {
            callbacks->resize_cb(window, width, height);
        }
    }

    void iconify_cb(GLFWwindow* window, int iconified) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks->iconify_cb) {
            callbacks->iconify_cb(window, iconified);
        }
    }

    void mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks->mouse_button_cb) {
            callbacks->mouse_button_cb(window, button, action, mods);
        }
    }

}

}
