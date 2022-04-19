import VkSurfaceKHR
import VkInstance

event MouseButton {
    int button
    int action
    int mods
    double x
    double y

    // in screen co-ords, from -1,-1 in top left to 1,1 in bottom right
    double x_rel
    double y_rel
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
    int width
    int height
}

request GetWindowFramebufferSize -> Extent2D {}

request CreateWindowSurface -> VkSurfaceKHR {
    VkInstance instance
}