load("//build_defs:bzl_utils.bzl", "rule_path")

CppSourceInfo = provider(
    "C++ source provider",
    fields = {
        "sources": "depset of sources",
        "headers": "depset of headers",
    },
)

def _impl(ctx):
    header = ctx.actions.declare_file(ctx.attr.header_name)
    source = ctx.actions.declare_file(rule_path(ctx) + ".cpp")

    args = ["-h", header.path, "-s", source.path]
    for src in ctx.files.srcs:
        args.append(src.path)

    ctx.actions.run(
        outputs = [header, source],
        inputs = ctx.files.srcs,
        tools = [ctx.executable._msg_lang_c],
        executable = ctx.executable._msg_lang_c,
        arguments = args,
        mnemonic = "MsgLangCppGen",
    )

    return CppSourceInfo(
        sources = depset([source]),
        headers = depset([header]),
    )

_msg_lang_cpp_gen = rule(
    implementation = _impl,
    attrs = {
        "srcs": attr.label_list(allow_files = [".msg"], allow_empty = False),
        "header_name": attr.string(),
        "_msg_lang_c": attr.label(executable = True, cfg = "exec", default = "//msg_lang:msg_lang_c"),
    },
    provides = [CppSourceInfo],
)

def _cpp_headers_impl(ctx):
    return DefaultInfo(
        files = ctx.attr.source_info[CppSourceInfo].headers,
    )

_cpp_headers = rule(
    implementation = _cpp_headers_impl,
    attrs = {
        "source_info": attr.label(providers = [CppSourceInfo])
    }
)

def _cpp_sources_impl(ctx):
    return DefaultInfo(
        files = ctx.attr.source_info[CppSourceInfo].sources,
    )

_cpp_sources = rule(
    implementation = _cpp_sources_impl,
    attrs = {
        "source_info": attr.label(providers = [CppSourceInfo])
    }
)

def msg_lang_cpp(name, srcs = [], **kwargs):
    _msg_lang_cpp_gen(
        name = "_{}_gen".format(name),
        header_name = "{}.h".format(name),
        srcs = srcs,
    )

    _cpp_headers(
        name = "_{}_headers".format(name),
        source_info = ":_{}_gen".format(name),
    )

    _cpp_sources(
        name = "_{}_sources".format(name),
        source_info = ":_{}_gen".format(name),
    )

    native.cc_library(
        name = name,
        srcs = [":_{}_sources".format(name)],
        hdrs = [":_{}_headers".format(name)],
        **kwargs,
    )
