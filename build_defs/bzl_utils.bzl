def rule_path(ctx):
    return "{}/{}".format(ctx.label.package, ctx.label.name)
