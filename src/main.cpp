#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

#include "dispatchers.h"
#include "scroller.h"

HANDLE PHANDLE = nullptr;
std::unique_ptr<ScrollerLayout> g_ScrollerLayout;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprscroller] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprscroller] Version mismatch");
    }

    g_ScrollerLayout = std::make_unique<ScrollerLayout>();
    HyprlandAPI::addLayout(PHANDLE, "scroller", g_ScrollerLayout.get());

    dispatchers::addDispatchers();

    // one value out of: { onesixth, onefourth, onethird, onehalf (default), twothirds, floating, maximized }
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_default_width", Hyprlang::STRING{"onehalf"});
    // one value out of: { onesixth, onefourth, onethird, onehalf, twothirds, one (default) }
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:window_default_height", Hyprlang::STRING{"one"});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:focus_wrap", Hyprlang::INT{1});
    // 0, inf
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:focus_edge_ms", Hyprlang::INT{400});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:cyclesize_wrap", Hyprlang::INT{1});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:cyclesize_closest", Hyprlang::INT{1});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:center_row_if_space_available", Hyprlang::INT{0});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:center_active_window", Hyprlang::INT{0});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:center_active_column", Hyprlang::INT{0});
    // 0, 1
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:overview_scale_content", Hyprlang::INT{1});
    // a list of values used as standard widths for cyclesize in row mode, and in the cycling order
    // available options: oneeighth, onesixth, onefourth, onethird, threeeighths, onehalf,
    // fiveeighths, twothirds, threequarters, fivesixths, seveneighths, one
    // default: onethird onehalf twothirds one
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:column_widths", Hyprlang::STRING{"onethird onehalf twothirds one"});
    // a list of values used as standard heights for cyclesize in column mode, and in the cycling order
    // available options: oneeighth, onesixth, onefourth, onethird, threeeighths, onehalf,
    // fiveeighths, twothirds, threequarters, fivesixths, seveneighths, one
    // default: onethird onehalf twothirds one
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:window_heights", Hyprlang::STRING{"onethird onehalf twothirds one"});
    // a list of values separated by ','. Each value is of the type MONITORID = ( options ),
    // where MONITORID is the name of a monitor;
    // options is a list of key = value separated by ';'.
    // Accepted keys are mode, column_default_width and window_default_height
    // default: empty, which means every monitor will get "row" mode and the default values
    // for the rest of the keys
    // monitor_options = ( DP-2 = ( mode = col; column_default_width = onethird; window_default_height = onehalf), HDMI-A-1 = ())
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:monitor_options", Hyprlang::STRING{""});
    //Enable gestures
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_sensitivity", Hyprlang::FLOAT{1.0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_distance", Hyprlang::INT{5});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_overview_fingers", Hyprlang::INT{4});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_scroll_enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_scroll_fingers", Hyprlang::INT{3});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_workspace_switch_enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_workspace_switch_distance", Hyprlang::INT{5});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_workspace_switch_fingers", Hyprlang::INT{4});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:gesture_workspace_switch_prefix", Hyprlang::STRING{""});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:col.selection_border", Hyprlang::INT{0xff9e1515});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:jump_labels_font", Hyprlang::STRING{""});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:jump_labels_scale", Hyprlang::FLOAT{0.5});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:jump_labels_color", Hyprlang::INT{0x80159e30});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:scroller:jump_labels_keys", Hyprlang::STRING{"1234"});

    HyprlandAPI::reloadConfig();

    return {"hyprscroller", "scrolling window layout", "dawser", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
