#include "sizes.h"

#include <sstream>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

extern HANDLE PHANDLE;

ScrollerSizes scroller_sizes;

StandardSize ScrollerSizes::get_next(const std::vector<StandardSize> &sizes, StandardSize size, int step) const
{
    int current = -1;
    for (size_t i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size) {
            current = i;
            break;
        }
    }
    if (current == -1) {
        return sizes[0];
    }
    int number = sizes.size();
    static auto* const *CYCLESIZE_WRAP = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:cyclesize_wrap")->getDataStaticPtr();
    if (**CYCLESIZE_WRAP)
        current = (number + current + step) % number;
    else {
        current += step;
        if (current < 0)
            current = 0;
        else if (current >= number)
            current = number - 1;
    }
    return sizes[current];
}

StandardSize ScrollerSizes::get_next_window_height(StandardSize size, int step)
{
    update();
    const std::string &monitor = g_pCompositor->m_pLastMonitor->szName;
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor)
            return get_next(monitor_data.window_heights, size, step);
    }
    return get_next(window_heights, size, step);
}

StandardSize ScrollerSizes::get_next_column_width(StandardSize size, int step)
{
    update();
    const std::string &monitor = g_pCompositor->m_pLastMonitor->szName;
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor)
            return get_next(monitor_data.column_widths, size, step);
    }
    return get_next(column_widths, size, step);
}

StandardSize ScrollerSizes::get_size(const std::vector<StandardSize> &sizes, int index) const
{
    int current = std::min(std::max(0, index), static_cast<int>(sizes.size()) - 1);
    return sizes[current];
}

StandardSize ScrollerSizes::get_window_height(int index)
{
    update();
    const std::string &monitor = g_pCompositor->m_pLastMonitor->szName;
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor)
            return get_size(monitor_data.window_heights, index);
    }
    return get_size(window_heights, index);
}

StandardSize ScrollerSizes::get_column_width(int index)
{
    update();
    const std::string &monitor = g_pCompositor->m_pLastMonitor->szName;
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor)
            return get_size(monitor_data.column_widths, index);
    }
    return get_size(column_widths, index);
}

// ScrollerSizes
Mode ScrollerSizes::get_mode(PHLMONITOR monitor)
{
    update();
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor->szName)
            return monitor_data.mode;
    }
    return Mode::Row;
}

StandardSize ScrollerSizes::get_window_default_height(PHLWINDOW window)
{
    // Check window rules
    for (auto &r: window->m_vMatchedRules) {
        if (r->szRule.starts_with("plugin:scroller:windowheight")) {
            const auto window_height = r->szRule.substr(r->szRule.find_first_of(' ') + 1);
            return get_size_from_string(window_height, StandardSize::One);
        }
    }
    const auto monitor = window->m_pMonitor.lock();
    update();
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor->szName)
            return monitor_data.window_default_height;
    }
    return window_default_height;
}

StandardSize ScrollerSizes::get_column_default_width(PHLWINDOW window)
{
    // Check window rules
    for (auto &r: window->m_vMatchedRules) {
        if (r->szRule.starts_with("plugin:scroller:columnwidth")) {
            const auto column_width = r->szRule.substr(r->szRule.find_first_of(' ') + 1);
            return get_size_from_string(column_width, StandardSize::OneHalf);
        }
    }
    const auto monitor = window->m_pMonitor.lock();
    update();
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor->szName)
            return monitor_data.column_default_width;
    }
    return column_default_width;
}

StandardSize ScrollerSizes::get_closest_size(const std::vector<StandardSize> &sizes, double fraction, int step) const
{
    size_t closest = step >= 0 ? sizes.size() - 1 : 0;
    double closest_distance = 2.0;
    for (size_t i = 0; i < sizes.size(); ++i) {
        double s;
        switch (sizes[i]) {
        case StandardSize::OneEighth:
            s = 1.0 / 8.0;
            break;
        case StandardSize::OneSixth:
            s = 1.0 / 6.0;
            break;
        case StandardSize::OneFourth:
            s = 1.0 / 4.0;
            break;
        case StandardSize::OneThird:
            s = 1.0 / 3.0;
            break;
        case StandardSize::ThreeEighths:
            s = 3.0 / 8.0;
            break;
        case StandardSize::OneHalf:
            s = 1.0 / 2.0;
            break;
        case StandardSize::FiveEighths:
            s = 5.0 / 8.0;
            break;
        case StandardSize::TwoThirds:
            s = 2.0 / 3.0;
            break;
        case StandardSize::ThreeQuarters:
            s = 3.0 / 4.0;
            break;
        case StandardSize::FiveSixths:
            s = 5.0 / 6.0;
            break;
        case StandardSize::SevenEighths:
            s = 7.0 / 8.0;
            break;
        case StandardSize::One:
            s = 1.0;
            break;
        default:
            // Shouldn't be here
            s = 2.0;
            break;
        }
        double distance = step * (s - fraction);
        if (distance >= 0.0 && distance < closest_distance) {
            closest = i;
            closest_distance = distance;
        }
    }
    return sizes[closest];
}

StandardSize ScrollerSizes::get_window_closest_height(PHLMONITORREF monitor, double fraction, int step)
{
    update();
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor->szName) {
            return get_closest_size(monitor_data.window_heights, fraction, step);
        }
    }
    return get_closest_size(window_heights, fraction, step);
}

StandardSize ScrollerSizes::get_column_closest_width(PHLMONITORREF monitor, double fraction, int step)
{
    update();
    for (const auto monitor_data : monitors) {
        if (monitor_data.name == monitor->szName) {
            return get_closest_size(monitor_data.column_widths, fraction, step);
        }
    }
    return get_closest_size(column_widths, fraction, step);
}

void ScrollerSizes::update_sizes(std::vector<StandardSize> &sizes, const std::string &option, StandardSize default_size)
{
    sizes.clear();
    std::string size;
    std::stringstream stream(option);
    while (std::getline(stream, size, ' ')) {
        if (size == "oneeighth") {
            sizes.push_back(StandardSize::OneEighth);
        } else if (size == "onesixth") {
            sizes.push_back(StandardSize::OneSixth);
        } else if (size == "onefourth") {
            sizes.push_back(StandardSize::OneFourth);
        } else if (size == "onethird") {
            sizes.push_back(StandardSize::OneThird);
        } else if (size == "threeeighths") {
            sizes.push_back(StandardSize::ThreeEighths);
        } else if (size == "onehalf") {
            sizes.push_back(StandardSize::OneHalf);
        } else if (size == "fiveeighths") {
            sizes.push_back(StandardSize::FiveEighths);
        } else if (size == "twothirds") {
            sizes.push_back(StandardSize::TwoThirds);
        } else if (size == "threequarters") {
            sizes.push_back(StandardSize::ThreeQuarters);
        } else if (size == "fivesixths") {
            sizes.push_back(StandardSize::FiveSixths);
        } else if (size == "seveneighths") {
            sizes.push_back(StandardSize::SevenEighths);
        } else if (size == "one") {
            sizes.push_back(StandardSize::One);
        }
    }
    // if sizes is wrong, use a default value
    if (sizes.size() == 0)
        sizes.push_back(default_size);
}

void ScrollerSizes::update()
{
    static auto const *window_default_height_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:window_default_height")->getDataStaticPtr();
    static auto const *column_default_width_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:column_default_width")->getDataStaticPtr();
    static auto const *window_heights_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:window_heights")->getDataStaticPtr();
    static auto const *column_widths_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:column_widths")->getDataStaticPtr();
    static auto const *monitor_modes_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:monitor_options")->getDataStaticPtr();

    if (*window_default_height_str == str_window_default_height &&
        *column_default_width_str == str_column_default_width &&
        *window_heights_str == str_window_heights &&
        *column_widths_str == str_column_widths &&
        *monitor_modes_str == str_monitors)
        return;

    window_default_height = get_size_from_string(*window_default_height_str, StandardSize::One);
    column_default_width = get_size_from_string(*column_default_width_str, StandardSize::OneHalf);
    update_sizes(window_heights, *window_heights_str, StandardSize::One);
    update_sizes(column_widths, *column_widths_str, StandardSize::OneHalf);
 
    str_window_default_height = *window_default_height_str;
    str_column_default_width = *column_default_width_str;
    str_window_heights = *window_heights_str;
    str_column_widths = *column_widths_str;
    str_monitors = *monitor_modes_str;

    monitors.clear();

    if (str_monitors != "") {
        std::string input = str_monitors;
        trim(input);
        size_t b = input.find_first_of('(');
        if (b != std::string::npos) {
            size_t e = input.find_last_of(')');
            if (e != std::string::npos) {
                std::string modes = input.substr(b + 1, e - b - 1);
                if (!modes.empty()) {
                    std::string monitor_mode;
                    std::stringstream stream(modes);
                    while (std::getline(stream, monitor_mode, ',')) {
                        size_t pos = monitor_mode.find('=');
                        if (pos != std::string::npos) {
                            std::string name = monitor_mode.substr(0, pos);
                            trim(name);
                            MonitorData monitor_data;
                            monitor_data.name = name;
                            monitor_data.mode = Mode::Row;
                            monitor_data.window_default_height =  window_default_height;
                            monitor_data.column_default_width = column_default_width;
                            monitor_data.window_heights = window_heights;
                            monitor_data.column_widths = column_widths;
                            std::string data = monitor_mode.substr(pos + 1);
                            b = data.find_first_of('(');
                            if (b != std::string::npos) {
                                e = data.find_last_of(')');
                                if (e != std::string::npos) {
                                    std::string options = data.substr(b + 1, e - b - 1);
                                    trim(options);
                                    if (!options.empty()) {
                                        std::string option;
                                        std::stringstream options_stream(options);
                                        while (std::getline(options_stream, option, ';')) {
                                            size_t pos = option.find('=');
                                            if (pos != std::string::npos) {
                                                std::string option_name = option.substr(0, pos);
                                                std::string option_data = option.substr(pos + 1);
                                                trim(option_name);
                                                trim(option_data);
                                                if (!option_data.empty()) {
                                                    if (option_name == "mode") {
                                                        if (option_data == "r" || option_data == "row") {
                                                            monitor_data.mode = Mode::Row;
                                                        } else if (option_data == "c" || option_data == "col" || option_data == "column") {
                                                            monitor_data.mode = Mode::Column;
                                                        }
                                                    } else if (option_name == "column_default_width") {
                                                        monitor_data.column_default_width = get_size_from_string(option_data, StandardSize::OneHalf);
                                                    } else if (option_name == "window_default_height") {
                                                        monitor_data.window_default_height = get_size_from_string(option_data, StandardSize::One);
                                                    } else if (option_name == "column_widths") {
                                                        update_sizes(monitor_data.column_widths, option_data, StandardSize::OneHalf);
                                                    } else if (option_name == "window_heights") {
                                                        update_sizes(monitor_data.window_heights, option_data, StandardSize::One);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            monitors.push_back(monitor_data);
                        }
                    }
                }
            }
        }
    }
}

StandardSize ScrollerSizes::get_size_from_string(const std::string &size, StandardSize default_size)
{
    if (size == "one") {
        return StandardSize::One;
    } else if (size == "oneeighth") {
        return StandardSize::OneEighth;
    } else if (size == "onesixth") {
        return StandardSize::OneSixth;
    } else if (size == "onefourth") {
        return StandardSize::OneFourth;
    } else if (size == "onethird") {
        return StandardSize::OneThird;
    } else if (size == "threeeighths") {
        return StandardSize::ThreeEighths;
    } else if (size == "onehalf") {
        return StandardSize::OneHalf;
    } else if (size == "fiveeighths") {
        return StandardSize::FiveEighths;
    } else if (size == "twothirds") {
        return StandardSize::TwoThirds;
    } else if (size == "threequarters") {
        return StandardSize::ThreeQuarters;
    } else if (size == "fivesixths") {
        return StandardSize::FiveSixths;
    } else if (size == "seveneighths") {
        return StandardSize::SevenEighths;
    } else {
        return default_size;
    }
}

void ScrollerSizes::trim(std::string &str)
{
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(),
                         [](unsigned char c) { return !std::isspace(c); }));
  str.erase(std::find_if(str.rbegin(), str.rend(),
                         [](unsigned char c) { return !std::isspace(c); })
                .base(),
            str.end());
}
