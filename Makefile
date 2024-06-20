clean:
	bazel clean --expunge --async --sandbox_debug
run:
	bazel run //src
compiledb:
	bazel run @hedron_compile_commands//:refresh_all
test:
	bazel run //tests --sandbox_debug
exp:
	bazel run //src:kqueue --sandbox_debug --verbose_failures