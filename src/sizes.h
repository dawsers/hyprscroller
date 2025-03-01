#ifndef SCROLLER_SIZES_H
#define SCROLLER_SIZES_H

#include <hyprland/src/Compositor.hpp>

#include <vector>

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

class ScrollerSizes {
public:
    ScrollerSizes() {}
    ~ScrollerSizes() {}

    Mode get_mode(PHLMONITOR monitor);
    StandardSize get_window_default_height(PHLWINDOW window);
    StandardSize get_column_default_width(PHLWINDOW window);
    StandardSize get_window_closest_height(PHLMONITORREF monitor, double fraction, int step);
    StandardSize get_column_closest_width(PHLMONITORREF monitor, double fraction, int step);
    StandardSize get_next_window_height(StandardSize size, int step);
    StandardSize get_next_column_width(StandardSize size, int step);
    StandardSize get_window_height(int index);
    StandardSize get_column_width(int index);
    StandardSize get_size_from_string(const std::string &size, StandardSize default_size);

private:
    StandardSize get_next(const std::vector<StandardSize> &sizes, StandardSize size, int step) const;
    StandardSize get_size(const std::vector<StandardSize> &sizes, int index) const;
    StandardSize get_closest_size(const std::vector<StandardSize> &sizes, double fraction, int step) const;
    void update();
    void update_sizes(std::vector<StandardSize> &sizes, const std::string &option, StandardSize default_size);
    void trim(std::string &str);

    std::string str_window_default_height;
    std::string str_column_default_width;
    std::string str_monitors;
    std::string str_window_heights;
    std::string str_column_widths;

    // Configuration globals
    StandardSize window_default_height;
    StandardSize column_default_width;
    std::vector<StandardSize> window_heights;
    std::vector<StandardSize> column_widths;

    // Per monitor configuration
    typedef struct {
        std::string name;
        Mode mode;
        StandardSize window_default_height;
        StandardSize column_default_width;
        std::vector<StandardSize> window_heights;
        std::vector<StandardSize> column_widths;
    } MonitorData;

    std::vector<MonitorData> monitors;
};


#endif // SCROLLER_SIZES_H
