load("@//build_defs:glsl.bzl", "spirv")

cc_binary(
    name = "application",
    srcs = glob(["*.cpp", "*.h"]),
    visibility = ["//visibility:public"],
    deps = ["//core_messages", "//framework", "//window", "//rendering", "//utils"],
    copts = ["-Werror"],
    data = [":shaders"],
)

spirv(
    name = "shaders",
    srcs = ["shader.vert.glsl", "shader.frag.glsl"],
)

