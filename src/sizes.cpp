#include "sizes.h"

#include <sstream>

extern HANDLE PHANDLE;

CycleSizes window_heights;
CycleSizes column_widths;
ScrollerSizes scroller_sizes;

StandardSize CycleSizes::get_next(StandardSize size, int step)
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

void CycleSizes::update(const std::string &option)
{
    if (option == str)
        return;
    reset();
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
    // if sizes is wrong, use a default value of onehalf
    if (sizes.size() == 0)
        sizes.push_back(StandardSize::OneHalf);

    str = option;
}

// ScrollerSizes
Mode ScrollerSizes::get_mode(PHLMONITOR monitor)
{
    update_sizes(monitor);
    const auto monitor_data = monitors.find(monitor->szName);
    if (monitor_data != monitors.end()) {
        return monitor_data->second.mode;
    } else {
        // shouldn't be here
        return Mode::Row;
    }
}

ConfigurationSize ScrollerSizes::get_window_default_height(PHLWINDOW window)
{
    // Check window rules
    for (auto &r: window->m_vMatchedRules) {
        if (r->szRule.starts_with("plugin:scroller:windowheight")) {
            const auto window_height = r->szRule.substr(r->szRule.find_first_of(' ') + 1);
            return get_window_default_height_fron_string(window_height);
        }
    }
    const auto monitor = window->m_pMonitor.lock();
    update_sizes(monitor);
    const auto monitor_data = monitors.find(monitor->szName);
    if (monitor_data != monitors.end()) {
        return monitor_data->second.window_default_height;
    } else {
        // shouldn't be here
        return ConfigurationSize::One;
    }
}

ConfigurationSize ScrollerSizes::get_column_default_width(PHLWINDOW window)
{
    // Check window rules
    for (auto &r: window->m_vMatchedRules) {
        if (r->szRule.starts_with("plugin:scroller:columnwidth")) {
            const auto column_width = r->szRule.substr(r->szRule.find_first_of(' ') + 1);
            return get_column_default_width_fron_string(column_width);
        }
    }
    const auto monitor = window->m_pMonitor.lock();
    update_sizes(monitor);
    const auto monitor_data = monitors.find(monitor->szName);
    if (monitor_data != monitors.end()) {
        return monitor_data->second.column_default_width;
    } else {
        // shouldn't be here
        return ConfigurationSize::OneHalf;
    }
}

void ScrollerSizes::update_sizes(PHLMONITOR monitor)
{
    MonitorData monitor_data;
    monitor_data.mode = Mode::Row;
    monitor_data.window_default_height = updatate_window_default_height();
    monitor_data.column_default_width = updatate_column_default_width();

    static auto const *monitor_modes_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:monitor_options")->getDataStaticPtr();
    if (monitor_modes_str != nullptr) {
        std::string input = *monitor_modes_str;
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
                            if (monitor->szName == name) {
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
                                                            monitor_data.column_default_width = get_column_default_width_fron_string(option_data);
                                                        } else if (option_name == "window_default_height") {
                                                            monitor_data.window_default_height = get_window_default_height_fron_string(option_data);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    monitors[monitor->szName] = monitor_data;
}

ConfigurationSize ScrollerSizes::get_window_default_height_fron_string(const std::string &window_height)
{
    if (window_height == "one") {
        return ConfigurationSize::One;
    } else if (window_height == "oneeighth") {
        return ConfigurationSize::OneEighth;
    } else if (window_height == "onesixth") {
        return ConfigurationSize::OneSixth;
    } else if (window_height == "onefourth") {
        return ConfigurationSize::OneFourth;
    } else if (window_height == "onethird") {
        return ConfigurationSize::OneThird;
    } else if (window_height == "threeeighths") {
        return ConfigurationSize::ThreeEighths;
    } else if (window_height == "onehalf") {
        return ConfigurationSize::OneHalf;
    } else if (window_height == "fiveeighths") {
        return ConfigurationSize::FiveEighths;
    } else if (window_height == "twothirds") {
        return ConfigurationSize::TwoThirds;
    } else if (window_height == "threequarters") {
        return ConfigurationSize::ThreeQuarters;
    } else if (window_height == "fivesixths") {
        return ConfigurationSize::FiveSixths;
    } else if (window_height == "seveneighths") {
        return ConfigurationSize::SevenEighths;
    } else {
        return ConfigurationSize::One;
    }
}

ConfigurationSize ScrollerSizes::updatate_window_default_height()
{
    static auto const *window_default_height = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:window_default_height")->getDataStaticPtr();
    std::string window_height = *window_default_height;
    return get_window_default_height_fron_string(window_height);
}

ConfigurationSize ScrollerSizes::get_column_default_width_fron_string(const std::string &column_width)
{
    if (column_width == "onehalf") {
        return ConfigurationSize::OneHalf;
    } else if (column_width == "oneeighth") {
        return ConfigurationSize::OneEighth;
    } else if (column_width == "onesixth") {
        return ConfigurationSize::OneSixth;
    } else if (column_width == "onefourth") {
        return ConfigurationSize::OneFourth;
    } else if (column_width == "onethird") {
        return ConfigurationSize::OneThird;
    } else if (column_width == "threeeighths") {
        return ConfigurationSize::ThreeEighths;
    } else if (column_width == "fiveeighths") {
        return ConfigurationSize::FiveEighths;
    } else if (column_width == "twothirds") {
        return ConfigurationSize::TwoThirds;
    } else if (column_width == "threequarters") {
        return ConfigurationSize::ThreeQuarters;
    } else if (column_width == "fivesixths") {
        return ConfigurationSize::FiveSixths;
    } else if (column_width == "seveneighths") {
        return ConfigurationSize::SevenEighths;
    } else if (column_width == "one") {
        return ConfigurationSize::One;
    } else if (column_width == "maximized") {
        return ConfigurationSize::Maximized;
    } else if (column_width == "floating") {
        return ConfigurationSize::Floating;
    } else {
        return ConfigurationSize::OneHalf;
    }
}

ConfigurationSize ScrollerSizes::updatate_column_default_width()
{
    static auto const *column_default_width = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:column_default_width")->getDataStaticPtr();
    std::string column_width = *column_default_width;
    return get_column_default_width_fron_string(column_width);
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
