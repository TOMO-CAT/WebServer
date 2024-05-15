includes("**/xmake.lua")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

-- 在 release 模式的基础上开启调试符号
-- xmake f -m releasedbg
add_rules("mode.releasedbg")
set_config("mode", "releasedbg")

set_languages("c++17")
add_includedirs(os.projectdir())

add_cxxflags("-Wall", "-Wextra", "-Werror")

add_requires("gtest 1.13.0", {configs = {main = true}})
