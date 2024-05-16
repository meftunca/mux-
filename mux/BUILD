load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "mux",
    hdrs = ["HTTP.hpp"],
    # -lpthread
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        "//ada",
        "//taskflow",
    ],
)
