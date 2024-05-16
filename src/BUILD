load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "src",
    srcs = ["http.cc"],
    copts = [
        "-std=c++20",
        "-O3",
        "-march=native",
        # "-flto",
        # "-fno-omit-frame-pointer",
        "-g",
        # "-fno-rtti",
        "-DNDEBUG",
    ],
    linkopts = ["-pthread"],
    deps = [
        "//mux",
    ],
)

cc_binary(
    name = "dynamic",
    srcs = ["dynamic.cc"],
    copts = [
        "-std=c++20",
    ],
    linkopts = ["-pthread"],
    deps = [
        "//mux",
    ],
)

cc_binary(
    name = "header",
    srcs = ["header.cc"],
    copts = [
        "-std=c++20",
    ],
    linkopts = ["-pthread"],
    deps = [
        "//ada",
        "//mux",
    ],
)
