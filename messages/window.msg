import VkSurfaceKHR
import VkInstance

event MouseButton {
    int button
    int action
    int mods
    double x
    double y
}

event KeyPress {
    int glfw_key
}

event WindowResize {
    int width
    int height
}

event WindowMinimised {}
event WindowRestored {}

data Extent2D {
    uint width
    uint height
}

request GetWindowFramebufferSize -> Extent2D {}

request CreateWindowSurface -> VkSurfaceKHR {
    VkInstance instance
}
