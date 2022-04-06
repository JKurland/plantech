load(":msg_lang.bzl", "msg_lang_cpp")
load("//build_defs:bzl_utils.bzl", "empty_file")

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


# test compilation of a msg_lang module

empty_file(
    name = "test_hdr",
    path = "test_empty.h"
)

cc_library(
    name = "test_lib",
    hdrs = [":test_hdr"]
)

msg_lang_cpp(
    name = "test_",
    srcs = ["tests/test.msg"],
    deps = [":test_lib"]
)
