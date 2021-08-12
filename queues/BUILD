cc_library(
    name = "queues",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    linkopts = ["-pthread"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":queues", "@com_google_googletest//:gtest_main"],
)

cc_binary(
    name = "bench",
    srcs = glob(["benchmarks/*.cpp"]),
    deps = [":queues", "@com_google_benchmark//:benchmark_main"],
)