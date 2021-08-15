cc_library(
    name = "framework",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    deps = ["//thread_pool"],
    visibility = ["//visibility:public"],
    copts = ["-Werror"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":framework", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)
