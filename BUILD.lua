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


local pdbdump = cc.binary {
    name = "pdbdump",
    srcs = glob {
        "src/pdbdump/*.cpp",
        "src/util/*.cpp",
        "src/util/*.c",
        "src/msf/*.cpp",
        "src/pdb/*.cpp",
    },
    src_deps = {
        ["src/pdbdump/main.cpp"] = {"src/version.h"},
    },
    includes = {"src"},
    warnings = {"all", "error"},
    compiler_opts = {"-g"},
}

--
-- Test pdbdump
--
rule {
    inputs = {pdbdump:path()},
    task = {{pdbdump:path(), "--help"}},
    outputs = {},
}

rule {
    inputs = {pdbdump:path()},
    task = {{pdbdump:path(), "--version"}},
    outputs = {},
}
