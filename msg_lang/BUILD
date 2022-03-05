cc_library(
    name = "msg_lang",
    copts = ["-Werror"],
    srcs = glob(["*.cpp"], exclude=["main.cpp"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "msg_lang_c",
    copts = ["-Werror"],
    srcs = ["main.cpp"],
    deps = [":msg_lang"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = glob(["tests/*.cpp"]),
    deps = [":msg_lang", "@com_google_googletest//:gtest_main"],
    copts = ["-Werror"],
)
