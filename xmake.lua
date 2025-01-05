add_rules("mode.release", "mode.debug")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if is_config("target_type", "server") then
    add_requires("levilamina 1.0.0-rc.1", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.0.0-rc.1", {configs = {target_type = "client"}})
end

add_requires("levibuildscript")
add_requires(
    "levibuildscript",
    "parallel-hashmap"
)

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()


target("LeviOptimize")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_defines(
        "_HAS_CXX23=1", -- To enable C++23 features
        "NOMINMAX", -- To avoid conflicts with std::min and std::max.
        "UNICODE" -- To enable Unicode support in Windows API.
    )
    add_files(
        "src/**.cpp"
    )
    add_includedirs(
        "src"
    )
    add_packages(
        "levilamina",
        "parallel-hashmap"
    )
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("cxx20")
    set_symbols("debug")
