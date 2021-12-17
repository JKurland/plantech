cc_library(
    name = "gui_manager",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//gui", "//framework"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":gui_manager", "//gui_rendering", "//gui", "//rendering", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)
