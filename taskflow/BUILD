load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "taskflow",
    hdrs = glob(["**/*.hpp"]),
    # -lpthread
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)
