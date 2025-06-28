add_rules("mode.debug", "mode.release")
set_languages("c17", "cxxlatest")
set_encodings("utf-8")

if is_plat("windows") then
    set_toolchains("msvc")
    add_cxflags("/GL")
    add_ldflags("/LTCG", "/OPT:REF", "/OPT:ICF")
    set_runtimes("MT")
end

if is_mode("debug") then
    set_symbols("hidden")
    set_optimize("none")
else
    set_optimize("smallest")
end

add_requires("vcpkg::spdlog",     {alias = "spdlog"})
add_requires("vcpkg::safetyhook", {alias = "safetyhook"})
add_requires("vcpkg::inipp",      {alias = "inipp"})
add_requires("vcpkg::fmt",        {alias = "fmt"})
add_requires("vcpkg::zydis",      {alias = "zydis"})

target("PhotoModePatches")
    set_kind("shared")
    set_extension(".asi")
    set_prefixname("")
    add_files("src/*.cpp")
    add_packages("safetyhook")
    add_packages("spdlog")
    add_packages("inipp")
    add_packages("fmt")
    add_packages("zydis")
    add_syslinks("user32")
