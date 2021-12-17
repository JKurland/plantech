load("@//build_defs:glsl.bzl", "spirv")

cc_library(
    name = "gui_rendering",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//gui", "@glm//:glm", "//rendering", "//framework"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    data = [":triangle_shaders"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":gui_rendering", "//gui", "//rendering", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

spirv(
    name = "triangle_shaders",
    srcs = ["triangle.vert.glsl", "triangle.frag.glsl"],
)
