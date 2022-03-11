load(":msg_lang.bzl", "msg_lang_cpp")

cc_library(
    name = "msg_lang",
    copts = ["-Werror", "-std=c++20"],
    srcs = glob(["*.cpp"], exclude=["main.cpp"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "msg_lang_c",
    copts = ["-Werror", "-std=c++20"],
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

msg_lang_cpp(
    name = "test_",
    srcs = ["test.msg"],
)
