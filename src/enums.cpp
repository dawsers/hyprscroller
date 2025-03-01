#include <hyprutils/string/VarList.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "enums.h"

extern HANDLE PHANDLE;

ModeModifier::ModeModifier() : position(POSITION_UNDEFINED), focus(FOCUS_UNDEFINED), auto_mode(AUTO_UNDEFINED), auto_param(2) {}

ModeModifier::ModeModifier(const std::string &arg) : ModeModifier() {
    const auto args = CVarList(arg);
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "after")
            set_position(ModeModifier::POSITION_AFTER);
        else if (args[i] == "before")
            set_position(ModeModifier::POSITION_BEFORE);
        else if (args[i] == "end")
            set_position(ModeModifier::POSITION_END);
        else if (args[i] == "beginning" || args[i] == "beg")
            set_position(ModeModifier::POSITION_BEGINNING);

        if (args[i] == "focus")
            set_focus(ModeModifier::FOCUS_FOCUS);
        else if (args[i] == "nofocus")
            set_focus(ModeModifier::FOCUS_NOFOCUS);

        const auto pos = args[i].find("auto:");
        if (pos != std::string::npos) {
            set_auto_mode(ModeModifier::AUTO_AUTO);
            set_auto_param(std::stoi(args[i].substr(pos + 5)));
        } else if (args[i] == "manual")
            set_auto_mode(ModeModifier::AUTO_MANUAL);

        if (args[i] == "center_column")
            set_center_column(true);
        else if (args[i] == "nocenter_column")
            set_center_column(false);
        if (args[i] == "center_window")
            set_center_window(true);
        else if (args[i] == "nocenter_window")
            set_center_window(false);
    }
}

void ModeModifier::set_position(int p) {
    position = p;
}

int ModeModifier::get_position(bool force_default) const {
    if (force_default && position == POSITION_UNDEFINED)
        return POSITION_AFTER;
    return position;
}

std::string ModeModifier::get_position_string() const {
    auto pos = get_position();
    switch (pos) {
    case POSITION_AFTER:
    default:
        return "after";
    case POSITION_BEFORE:
        return "before";
    case POSITION_END:
        return "end";
    case POSITION_BEGINNING:
        return "beginning";
    }
}

void ModeModifier::set_focus(int f) {
    focus = f;
}

int ModeModifier::get_focus(bool force_default) const {
    if (force_default && focus == FOCUS_UNDEFINED)
        return FOCUS_FOCUS;
    return focus;
}

std::string ModeModifier::get_focus_string() const {
    if (get_focus() == FOCUS_FOCUS)
        return "focus";
    return "nofocus";
}

void ModeModifier::set_auto_mode(int mode) {
    auto_mode = mode;
}

void ModeModifier::set_auto_param(int param) {
    auto_param = param;
}

int ModeModifier::get_auto_mode(bool force_default) const {
    if (force_default && auto_mode == AUTO_UNDEFINED)
        return AUTO_MANUAL;
    return auto_mode;
}

std::string ModeModifier::get_auto_mode_string() const {
    if (get_auto_mode() == AUTO_MANUAL)
        return "manual";
    return "auto";
}

int ModeModifier::get_auto_param() const {
    return auto_param;
}

void ModeModifier::set_center_column(bool c) {
    center_column = c;
}

std::optional<bool> ModeModifier::get_center_column(bool force_default) const {
    if (force_default && !center_column.has_value()) {
        static auto* const *center_active_column = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:center_active_column")->getDataStaticPtr();
        return **center_active_column;
    }
    return center_column;
}

std::string ModeModifier::get_center_column_string() const {
    if (get_center_column() == false)
        return "nocenter_column";
    return "center_column";
}

void ModeModifier::set_center_window(bool c) {
    center_window = c;
}

std::optional<bool> ModeModifier::get_center_window(bool force_default) const {
    if (force_default && !center_window.has_value()) {
        static auto* const *center_active_window = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:center_active_window")->getDataStaticPtr();
        return **center_active_window;
    }
    return center_window;
}

std::string ModeModifier::get_center_window_string() const {
    if (get_center_window() == false)
        return "nocenter_window";
    return "center_window";
}
