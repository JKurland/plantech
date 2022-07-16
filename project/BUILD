cc_library(
    name = "project",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    visibility = ["//visibility:public"],
    deps = ["//messages", "//framework"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = ["//test_utils", "//test_steps"],
    copts = ["-Werror"],
)

