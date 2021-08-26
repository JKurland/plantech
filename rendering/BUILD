load("@//build_defs:glsl.bzl", "spirv")

cc_library(
    name = "rendering",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//core_messages", "//framework", "@glm//:glm", "//window"],
    linkopts = ["-ldl", "-lX11", "-lvulkan"],
    copts = ["-Werror"],
    data = [":mesh_shaders"],
    visibility = ["//visibility:public"],
)

spirv(
    name = "mesh_shaders",
    srcs = ["mesh.vert.glsl", "mesh.frag.glsl"],
)

