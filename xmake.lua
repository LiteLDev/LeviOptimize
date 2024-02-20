add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires(
    "levilamina 0.8.2",
    "parallel-hashmap v1.3.12"
)

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("LeviOptimize")
    add_cxflags(
        "/EHa", -- To catch both structured (asynchronous) and standard C++ (synchronous) exceptions.
        "/utf-8" -- To enable UTF-8 source code.
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
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll" -- To use forged symbols of SymbolProvider.
    )
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("cxx20")
    set_symbols("debug")

    after_build(function (target)
        local plugin_packer = import("scripts.after_build")

        local plugin_define = {
            pluginName = target:name(),
            pluginFile = path.filename(target:targetfile()),
        }
        
        plugin_packer.pack_plugin(target,plugin_define)
    end)
