cc_library(
    name = "utils",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":utils", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)

