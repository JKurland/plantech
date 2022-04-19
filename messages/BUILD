load("//msg_lang:msg_lang.bzl", "msg_lang_cpp")

msg_lang_cpp(
    name = "messages",
    srcs = glob(["*.msg"]),
    deps = ["//gui"],
    system_hdrs = ["vulkan/vulkan.h"],
    linkopts = ["-lvulkan"],
    visibility = ["//visibility:public"],
)

