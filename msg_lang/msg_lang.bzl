load("//build_defs:bzl_utils.bzl", "rule_path")

CppSourceInfo = provider(
    "C++ source provider",
    fields = {
        "sources": "depset of sources",
        "headers": "depset of headers",
    },
)

def find_include_suffix(hdr, include_paths):
    "Finds the suffix of hdr that is relative to a path in include_paths"
    for path in include_paths.to_list():
        if not path.endswith("/"):
            dir_path = path + "/"
        else:
            dir_path = path
 
        if hdr.startswith(dir_path):
            return hdr[len(dir_path):]

    return hdr


def _impl(ctx):
    header = ctx.actions.declare_file(ctx.attr.header_name)
    source = ctx.actions.declare_file(rule_path(ctx) + ".cpp")

    args = ["-h", header.path, "-s", source.path]

    if ctx.attr.deps:
        for dep in ctx.attr.deps:
            include_paths = dep[CcInfo].compilation_context.quote_includes 
            for hdr in dep[CcInfo].compilation_context.direct_public_headers:
                args.append("-i")
                args.append(find_include_suffix(hdr.path, include_paths))

    if ctx.attr.system_hdrs:
        for h in ctx.attr.system_hdrs:
            args.append("--system_header")
            args.append(h)

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
        "deps": attr.label_list(providers = [CcInfo]),
        "system_hdrs": attr.string_list(),
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

def msg_lang_cpp(name, srcs = [], deps = None, system_hdrs = None, **kwargs):
    _msg_lang_cpp_gen(
        name = "_{}_gen".format(name),
        header_name = "{}.h".format(name),
        srcs = srcs,
        deps = deps,
        system_hdrs = system_hdrs,
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
        deps = deps,
        **kwargs,
    )
