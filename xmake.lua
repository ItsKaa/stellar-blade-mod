add_rules("mode.debug", "mode.release")
--set_warnings("all", "error")
set_languages("clatest", "cxxlatest")
set_encodings("utf-8")
set_optimize("smallest")

add_requires("vcpkg::spdlog",     {alias = "spdlog"})
add_requires("vcpkg::safetyhook", {alias = "safetyhook"})
add_requires("vcpkg::inipp",      {alias = "inipp", configs = { features = {"wchar"} }})
add_requires("vcpkg::fmt",        {alias = "fmt"})
add_requires("vcpkg::zydis",      {alias = "zydis"})

target("PhotoModePatches")
    set_kind("shared")
    set_prefixname("")
    add_files("src/*.cpp")
    add_packages("safetyhook")
    add_packages("spdlog")
    add_packages("inipp")
    add_packages("fmt")
    add_packages("zydis")
    add_syslinks("user32")


if is_plat("windows") then
    set_extension(".asi")
    set_toolchains("msvc")
end