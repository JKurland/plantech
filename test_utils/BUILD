cc_library(
    name = "test_utils",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//framework", "@com_google_googletest//:gtest_main"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = ["//test_utils"],
    copts = ["-Werror"],
)
