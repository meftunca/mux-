load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
    name = "src",
    srcs = ["http.cc"],
    copts = [
        "-std=c++20",
        "-O0",
        "-march=native",
        # "-flto",
        # "-fno-omit-frame-pointer",
        "-fno-rtti",
        # "-DNDEBUG",
        # "-ftree-vectorizer-verbose",
        # "-pg",
    ],
    linkopts = [
        "-pthread",
        "-L/opt/homebrew/opt/gperftools/lib",
    ],
    deps = [
        # ":gperftools",
        "//ada",
        "//mux",
    ],
)

cc_binary(
    name = "dynamic",
    srcs = ["dynamic.cc"],
    copts = [
        "-std=c++20",
    ],
    linkopts = [
        "-pthread",
        "-L/opt/homebrew/opt/gperftools/lib",
    ],
    deps = [
        "//ada",
        "//mux",
    ],
)

cc_binary(
    name = "kqueue",
    srcs = ["kqueue.cc"],
    copts = [
        "-std=c++20",
        "-O3",
        "-g",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Wno-unused-function",
        "-Wno-unused-but-set-variable",
        "-fexperimental-library",
        # "-fomit-frame-pointer",
        # "-fsanitize=address",
        # "-fsanitize=undefined",
        # "-fno-omit-frame-pointer",
        # "-fsanitize=integer",
        # "-fsanitize=nullability",
        # "-fsanitize=implicit-conversion",
        # "-fsanitize-recover=address",
        # "-fsanitize-address-use-after-scope",
        # "-D_GLIBCXX_DEBUG",
        # "-fstack-protector-all",
        "-march=native",
        # # network optimization
        "-fno-rtti",
        "-fno-omit-frame-pointer",
        # "-DNDEBUG",
        "-ftree-vectorizer-verbose",
        # "-pg",
    ],
    data = ["@llvm_toolchain//:linker-files-aarch64-darwin"],
    defines = [
        "detect_leaks=1",
        "TF_ENABLE_PROFILER=/Users/mapletechnologies/Desktop/big_projects/muxpp/simple.json",
    ],
    linkopts = [
        "-pthread",
        "-L/opt/homebrew/opt/gperftools/lib",
    ],
    # malloc = "@com_google_tcmalloc//tcmalloc",
    deps = [
        "//Socket",
        "//ada",
        "//mux",
        # "@com_google_absl//absl/strings",
        # "@com_google_absl//absl/strings:str_format",
        # "@com_google_absl//absl/types:optional",
        # "@com_google_tcmalloc//tcmalloc:malloc_extension",
    ],
)

cc_binary(
    name = "socket",
    srcs = ["Socket.cc"],
    copts = [
        "-std=c++20",
        "-O3",
    ],
    linkopts = [
        "-pthread",
        "-L/opt/homebrew/opt/gperftools/lib",
    ],
    deps = [
        "//Socket",
        "//ada",
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
