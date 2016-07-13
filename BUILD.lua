local cc = require "rules.cc"

cc.binary {
    name = "peclean",
    srcs = glob "src/*.cpp",
    warnings = {"all", "error"},
    compiler_opts = {"-g"},
}
