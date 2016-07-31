local cc = require "rules.cc"

local peclean = cc.binary {
    name = "peclean",
    srcs = glob { "src/*.cpp", "src/*.c" },
    warnings = {"all", "error"},
    compiler_opts = {"-g"},
}

--
-- Test peclean
--
local tests = {
    {image = "vs/vs2015/Debug/test_dll.dll", pdb = "vs/vs2015/Debug/test_dll.pdb"},
    {image = "vs/vs2015/Release/test_dll.dll", pdb = "vs/vs2015/Release/test_dll.pdb"},
    {image = "vs/vs2015/x64/Debug/test_dll.dll", pdb = "vs/vs2015/x64/Debug/test_dll.pdb"},
    {image = "vs/vs2015/x64/Release/test_dll.dll", pdb = "vs/vs2015/x64/Release/test_dll.pdb"},

    {image = "vs/vs2015/Debug/test_exe.exe", pdb = "vs/vs2015/Debug/test_exe.pdb"},
    {image = "vs/vs2015/Release/test_exe.exe", pdb = "vs/vs2015/Release/test_exe.pdb"},
    {image = "vs/vs2015/x64/Debug/test_exe.exe", pdb = "vs/vs2015/x64/Debug/test_exe.pdb"},
    {image = "vs/vs2015/x64/Release/test_exe.exe", pdb = "vs/vs2015/x64/Release/test_exe.pdb"},
}

for _,t in ipairs(tests) do
    rule {
        inputs = {peclean:path()},
        task = {{peclean:path(), t.image, t.pdb}},
        outputs = {},
    }
end

rule {
    inputs = {peclean:path()},
    task = {{peclean:path(), "--help"}},
    outputs = {},
}
