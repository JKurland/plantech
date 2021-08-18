cc_binary(
    name = "application",
    srcs = glob(["*.cpp"]),
    visibility = ["//visibility:public"],
    deps = ["//core_messages", "//framework", "//window", "//rendering"],
    copts = ["-Werror"],
)
