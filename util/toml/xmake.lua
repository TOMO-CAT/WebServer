target("util.toml", function()
    set_kind("object")
    add_rules("c++")
    add_deps("thirdparty.cpptoml")
end)
