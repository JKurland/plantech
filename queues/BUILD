cc_library(
    name = "queues",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    linkopts = ["-pthread"],
    copts = ["-Werror"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":queues", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

cc_binary(
    name = "bench",
    srcs = glob(["benchmarks/*.cpp"]),
    deps = [":queues", "@com_google_benchmark//:benchmark_main"],
    copts = ["-Werror"],
)