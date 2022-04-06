def rule_path(ctx):
    return ctx.label.name

def empty_file(name, path):
    native.genrule(
        name = name,
        outs = [path],
        cmd = '> "$@"'
    )
