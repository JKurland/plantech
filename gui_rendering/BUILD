cc_library(
    name = "gui_rendering",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//gui"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":gui_rendering", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

