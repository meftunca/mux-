load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "Socket",
    srcs = glob(["**/*.cc"]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    # -lpthread
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        "//taskflow",
    ],
)
