cc_library(
    name = "window",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    visibility = ["//visibility:public"],
    deps = ["@glfw//:glfw", "//core_messages", "//framework"],
    linkopts = ["-ldl", "-lX11"],
    copts = ["-Werror"],
)

