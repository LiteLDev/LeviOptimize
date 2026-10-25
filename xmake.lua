add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")
add_repositories("miracleforest-repo https://github.com/MiracleForest/xmake-repo.git")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

add_requires("levilamina 7c0f7d125a80274f406c0be8a9a17e4d9cbb7801", { configs = { target_type = get_config("target_type") } } )
add_requires("ipatch 1.0.0",  { configs = { shared = false } } )
add_requires("levibuildscript")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("LeviOptimize")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags(
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_cxflags(
        "/EHs",
        "-Wno-microsoft-cast",
        "-Wno-invalid-offsetof",
        "-Wno-c++2b-extensions",
        "-Wno-microsoft-include",
        "-Wno-overloaded-virtual",
        "-Wno-ignored-qualifiers",
        "-Wno-missing-field-initializers",
        "-Wno-potentially-evaluated-expression",
        "-Wno-pragma-system-header-outside-header",
        { tools = { "clang_cl" } }
    )
    add_defines(
        "NOMINMAX",
        "UNICODE",
        "_HAS_CXX23=1",
        "_SILENCE_CXX20_IS_ALWAYS_EQUAL_DEPRECATION_WARNING=1"
    )
    set_toolchains("clang-cl")
    add_packages("levilamina", "ipatch")
    set_kind("shared")
    set_exceptions("none")
    set_languages("c++23")
    set_symbols("debug")

    add_includedirs("src")
    add_headerfiles("src/**.h")
    add_files("src/lo/**.cpp")

    local platform_dir = "src-" .. (get_config("target_type") or "server")
    if os.isdir(platform_dir) then
        add_includedirs(platform_dir)

        local platform_headers = os.files(platform_dir .. "/**.h")
        if #platform_headers > 0 then
            add_headerfiles(table.unpack(platform_headers))
        end

        local platform_sources = os.files(platform_dir .. "/lo/**.cpp")
        if #platform_sources > 0 then
            add_files(table.unpack(platform_sources))
        end
    end

    after_build(function (target)
        local output_dir = path.join(os.projectdir(), "bin", target:name())
        os.vcp(path.join(os.projectdir(), "assets", "*"), format("%s/", output_dir))
    end)