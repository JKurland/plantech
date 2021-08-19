load("@//build_defs:glsl.bzl", "spirv")

cc_test(
    name = "test",
    srcs = glob(["*.cpp", "*.h"]),
    deps = ["//core_messages", "//framework", "//window", "//rendering", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
    data = [":shaders"],
)

spirv(
    name = "shaders",
    srcs = ["shader.vert.glsl", "shader.frag.glsl"],
)
