#include "window/window.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <atomic>


namespace pt {

Window::Window(int initial_width, int initial_height, std::string_view title) {
    stop_poll = std::make_unique<std::atomic<bool>>(false);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window.reset(glfwCreateWindow(initial_width, initial_height, title.data(), nullptr, nullptr));
}

Window::~Window() {
    if (poll_thread.joinable()) {
        *stop_poll = true;
        glfwPostEmptyEvent();
        poll_thread.join();
    }
}

namespace window::detail {
    void WindowDelete::operator()(GLFWwindow* window){
        glfwDestroyWindow(window);
    }

    GlfwInitRaii::GlfwInitRaii() {
        owns = true;
        glfwInit();
    }
    GlfwInitRaii::~GlfwInitRaii() {
        if (owns) {
            glfwTerminate();
        }
    }

    GlfwInitRaii::GlfwInitRaii(GlfwInitRaii&& o) {
        owns = true;
        o.owns = false;
    }

    GlfwInitRaii& GlfwInitRaii::operator=(GlfwInitRaii&& o) {
        std::swap(o.owns, owns);
        return *this;
    }


}

}
