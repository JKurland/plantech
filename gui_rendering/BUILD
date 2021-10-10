cc_library(
    name = "gui_rendering",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//gui", "@glm//:glm", "//rendering"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":gui_rendering", "//framework", "//gui", "//rendering", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

