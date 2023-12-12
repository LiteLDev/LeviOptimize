#include <ll/api/plugin/Plugin.h>

#include <Plugin.h>

namespace plugins {

// The global plugin instance.
Plugin plugin;

extern "C" {
_declspec(dllexport) bool ll_plugin_load(ll::plugin::Plugin& self) { return plugin.load(self); }

_declspec(dllexport) bool ll_plugin_unload(ll::plugin::Plugin& self) { return plugin.unload(self); }

_declspec(dllexport) bool ll_plugin_enable(ll::plugin::Plugin& self) { return plugin.enable(self); }

_declspec(dllexport) bool ll_plugin_disable(ll::plugin::Plugin& self) { return plugin.disable(self); }
}

} // namespace plugins
