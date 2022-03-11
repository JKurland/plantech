load("//build_defs:bzl_utils.bzl", "rule_path")

def _impl(ctx):
    output = ctx.actions.declare_file(rule_path(ctx) + ".cpp")

    args = ["-o", output.path]
    for src in ctx.files.srcs:
        args.append(src.path)

    ctx.actions.run(
        outputs = [output],
        inputs = ctx.files.srcs,
        tools = [ctx.executable._msg_lang_c],
        executable = ctx.executable._msg_lang_c,
        arguments = args,
        mnemonic = "MsgLangCppGen",
    )

    return DefaultInfo(files = depset([output]))

_msg_lang_cpp_gen = rule(
    implementation = _impl,
    attrs = {
        "srcs": attr.label_list(allow_files = [".msg"], allow_empty = False),
        "_msg_lang_c": attr.label(executable = True, cfg = "exec", default = "//msg_lang:msg_lang_c"),
    },
)

def msg_lang_cpp(name, srcs = [], **kwargs):
    _msg_lang_cpp_gen(
        name = "_{}_srcs".format(name),
        srcs = srcs,
    )

    native.cc_library(
        name = name,
        srcs = [":_{}_srcs".format(name)],
        **kwargs,
    )
