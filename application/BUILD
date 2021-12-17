cc_binary(
    name = "application",
    srcs = glob(["*.cpp", "*.h"]),
    visibility = ["//visibility:public"],
    deps = ["//core_messages", "//framework", "//window", "//rendering", "//utils", "//gui_rendering", "//gui_manager"],
    copts = ["-Werror"],
)


