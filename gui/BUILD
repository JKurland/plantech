cc_library(
    name = "gui",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//utils", "@glm//:glm"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":gui", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

