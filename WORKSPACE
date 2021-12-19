load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
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

http_archive(
  name = "com_google_googletest",
  urls = ["https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip"],
  strip_prefix = "googletest-609281088cfefc76f9d0ce82e1ff6c30cc3591e5",
)

http_archive(
  name = "rules_cc",
  urls = ["https://github.com/bazelbuild/rules_cc/archive/40548a2974f1aea06215272d9c2b47a14a24e556.zip"],
  strip_prefix = "rules_cc-40548a2974f1aea06215272d9c2b47a14a24e556",
)

http_archive(
  name = "com_google_benchmark",
  urls = ["https://github.com/google/benchmark/archive/refs/tags/v1.5.5.tar.gz"],
  strip_prefix = "benchmark-1.5.5",
)

# Hedron's Compile Commands Extractor for Bazel
# https://github.com/hedronvision/bazel-compile-commands-extractor
http_archive(
    name = "hedron_compile_commands",

    # Replace the commit hash in both places (below) with the latest. 
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" below).
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/486d6eaf87255b7e15640bb9add58f0a77c1e2b6.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-486d6eaf87255b7e15640bb9add58f0a77c1e2b6",
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()
