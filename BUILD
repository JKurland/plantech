load("@//build_defs:glsl.bzl", "spirv")

cc_binary(
    name = "vulkan_tut",
    srcs = ["vulkan_tut.cpp"],
    deps = ["@glfw//:glfw", "@glm//:glm"],
    data = [":shaders"],
    linkopts = ["-lX11", "-ldl", "-pthread", "-lvulkan"],
)

spirv(
    name = "shaders",
    srcs = ["shader.vert.glsl", "shader.frag.glsl"],
)
