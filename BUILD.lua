local cc = require "rules.cc"

-- FIXME: Implicit dependency detection goes a bit overboard on this. Fix this
-- once there is a way to override dependency detection.
rule {
    inputs = glob {
        "VERSION",
        "src/version.h.in",
    },
    task = {{"./scripts/version.py", "src/version.h.in", "src/version.h"}},
    outputs = {"src/version.h"},
}

local ducible = cc.binary {
    name = "ducible",
    srcs = glob {
        "src/util/*.cpp",
        "src/util/*.c",
        "src/msf/*.cpp",
        "src/ducible/*.cpp",
        "src/pe/*.cpp",
        "src/pdb/*.cpp",
    },
    src_deps = {
        ["src/ducible/main.cpp"] = {"src/version.h"},
    },
    includes = {"src"},
    warnings = {"all", "error"},
    compiler_opts = {"-g"},
}

--
-- Test ducible
--
rule {
    inputs = {ducible:path()},
    task = {{ducible:path(), "--help"}},
    outputs = {},
}

rule {
    inputs = {ducible:path()},
    task = {{ducible:path(), "--version"}},
    outputs = {},
}
