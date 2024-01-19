add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_requires("levilamina 0.5.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("LeviOptimize")
    add_cxflags(
        "/utf-8",
        "/permissive-",
        "/EHa",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_files(
        "src/**.cpp"
    )
    add_includedirs(
        "src"
    )
    add_packages(
        "levilamina"
    )
    add_rules(
        "mode.release"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll"
    )
    add_defines(
        "_HAS_CXX23=1"
    )
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")

    after_build(function (target)
        local plugin_packer = import("scripts.plugin_packer")

        local plugin_define = {
            pluginName = target:name(),
            pluginFile = path.filename(target:targetfile()),
        }
        
        plugin_packer.pack_plugin(target,plugin_define)
    end)
