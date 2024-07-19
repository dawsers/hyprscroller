#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "dispatchers.h"
#include "hyprlang.hpp"
#include "scroller.h"

HANDLE PHANDLE = nullptr;
std::unique_ptr<ScrollerLayout> g_ScrollerLayout;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

#ifdef COLORS_IPC
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:col.freecolumn_border", Hyprlang::CConfigValue(Hyprlang::INT(0xff9e1515)));
#endif
    g_ScrollerLayout = std::make_unique<ScrollerLayout>();
    HyprlandAPI::addLayout(PHANDLE, "scroller", g_ScrollerLayout.get());

    dispatchers::addDispatchers();

    // one value out of: { onethird, onehalf (default), twothirds, floating, maximized }
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_default_width", Hyprlang::STRING{"onehalf"});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:focus_wrap", Hyprlang::INT{1});
    // a list of values used as standard widths for cyclesize in row mode, and in the cycling order
    // available options: onesixth, onefourth, onethird, onehalf, twothirds, one
    // default: onehalf twothirds onethird
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_widths", Hyprlang::STRING{"onehalf twothirds onethird"});
    // a list of values used as standard heights for cyclesize in column mode, and in the cycling order
    // available options: onesixth, onefourth, onethird, onehalf, twothirds, one
    // default: one onethird onehalf twothirds
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:window_heights", Hyprlang::STRING{"one onethird onehalf twothirds"});

    HyprlandAPI::reloadConfig();

    return {"hyprscroller", "scrolling window layout", "dawser", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
