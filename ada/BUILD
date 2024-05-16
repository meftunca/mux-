load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ada",
    srcs = ["ada.cpp"],
    hdrs = ["ada.h"],
    # -lpthread
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)
