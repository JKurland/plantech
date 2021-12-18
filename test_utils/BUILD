cc_library(
    name = "test_utils",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.cpp"]),
    deps = ["//framework", "@com_google_googletest//:gtest"],
    linkopts = ["-ldl"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
)
