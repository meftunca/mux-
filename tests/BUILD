load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

# gperftools load from brew
# cc_library(
#     name = "gperftools",
#     hdrs = glob(["/opt/homebrew/opt/gperftools/include/gperftools/**"]),
#     includes = ["/opt/homebrew/opt/gperftools/include"],
#     linkopts = ["-L/opt/homebrew/opt/gperftools/lib"],
#     visibility = ["//visibility:public"],
# )

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["**/*.cc"]),
    copts = [
        "-std=c++2b",
        "-O0",
        "-march=native",
        # "-flto",
        # "-fno-omit-frame-pointer",
        "-fno-rtti",
        # "-DNDEBUG",
        # "-ftree-vectorizer-verbose",
        # "-pg",
    ],
    defines = ["CATCH_CONFIG_MAIN"],
    linkopts = [
        "-pthread",
        "-L/opt/homebrew/opt/gperftools/lib",
    ],
    deps = [
        # ":gperftools",
        "@catch2",
    ],
)
