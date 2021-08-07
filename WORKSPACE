load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository", "git_repository")

new_git_repository(
    name = "glfw",
    remote = "https://github.com/glfw/glfw.git",
    commit = "814b7929c5add4b0541ccad26fb81f28b71dc4d8",
    build_file = "//third_party:glfw.BUILD",
)

new_git_repository(
    name = "glm",
    remote = "https://github.com/g-truc/glm.git",
    commit = "bf71a834948186f4097caa076cd2663c69a10e1e",
    build_file = "//third_party:glm.BUILD",
)

git_repository(
    name = "glslang",
    remote = "https://github.com/KhronosGroup/glslang.git",
    commit = "bcf6a2430e99e8fc24f9f266e99316905e6d5134",
    shallow_since = "1587975125 -0600",
)
