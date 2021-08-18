cc_library(
    name = "rendering",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//core_messages", "//framework", "@glm//:glm", "//window"],
    linkopts = ["-ldl", "-lX11", "-lvulkan"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)


