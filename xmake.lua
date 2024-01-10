add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_requires("levilamina 0.4.0")

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
    add_cxxflags(
        "-Wno-c++2b-extensions",
        "-Wno-microsoft-cast",
        "-Wno-pragma-system-header-outside-header",
        {tools = {"clang_cl"}}
    )
    add_defines(
        "_AMD64_",
        "_CRT_SECURE_NO_WARNINGS",
        "_ENABLE_CONSTEXPR_MUTEX_CONSTRUCTOR",
        "NOMINMAX",
        "UNICODE",
        "WIN32_LEAN_AND_MEAN",
        "ENTT_PACKED_PAGE=128",
        "_HAS_CXX23=1"
    )
    add_files(
        "src/**.cpp"
    )
    add_includedirs(
        "src","include"
    )
    add_packages(
        "levilamina"
    )
    add_rules(
        "mode.debug",
        "mode.release",
        "mode.releasedbg"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll"
    )
    add_undefines(
        "_DEBUG"
    )
    set_exceptions("none")
    set_kind("shared")
    set_languages("cxx20")
    set_strip("all")

    after_build(function (target)
        local plugin_packer = import("scripts.plugin_packer")

        local plugin_define = {
            pluginName = target:name(),
            pluginFile = path.filename(target:targetfile()),
        }
        
        plugin_packer.pack_plugin(target,plugin_define)
    end)
