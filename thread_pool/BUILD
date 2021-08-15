cc_library(
    name = "thread_pool",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    linkopts = ["-pthread"],
    deps = ["//queues"],
    copts = ["-Werror"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":thread_pool", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)
