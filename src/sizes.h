#ifndef SCROLLER_SIZES_H
#define SCROLLER_SIZES_H

#include <hyprland/src/Compositor.hpp>

#include <vector>
#include <unordered_map>

#include "enums.h"

enum class StandardSize {
    OneEighth = 0,
    OneSixth,
    OneFourth,
    OneThird,
    ThreeEighths,
    OneHalf,
    FiveEighths,
    TwoThirds,
    ThreeQuarters,
    FiveSixths,
    SevenEighths,
    One,
    Free
};

enum class Reorder {
    Auto,
    Lazy
};


class CycleSizes {
public:
    CycleSizes() {}
    ~CycleSizes() { reset(); }
    StandardSize get_default() { return sizes[0]; }
    StandardSize get_next(StandardSize size, int step) const;
    StandardSize get_size(int index) const;
    void update(const std::string &option);

private:
    void reset() {
        sizes.clear();
    }

    std::string str;
    std::vector<StandardSize> sizes;
};


enum class ConfigurationSize {
    OneEighth = 0,
    OneSixth,
    OneFourth,
    OneThird,
    ThreeEighths,
    OneHalf,
    FiveEighths,
    TwoThirds,
    ThreeQuarters,
    FiveSixths,
    SevenEighths,
    One,
    Maximized,
    Floating
};

class ScrollerSizes {
public:
    ScrollerSizes() {}
    ~ScrollerSizes() { reset(); }

    Mode get_mode(PHLMONITOR monitor);
    ConfigurationSize get_window_default_height(PHLWINDOW window);
    ConfigurationSize get_column_default_width(PHLWINDOW window);

private:
    void update_sizes(PHLMONITOR monitor);
    void reset() {
        monitors.clear();
    }
    ConfigurationSize get_window_default_height_fron_string(const std::string &window_height);
    ConfigurationSize updatate_window_default_height();
    ConfigurationSize get_column_default_width_fron_string(const std::string &column_width);
    ConfigurationSize updatate_column_default_width();

    void trim(std::string &str);

    typedef struct {
        Mode mode;
        ConfigurationSize window_default_height;
        ConfigurationSize column_default_width;
    } MonitorData;

    std::unordered_map<std::string, MonitorData> monitors;
};


#endif // SCROLLER_SIZES_H
