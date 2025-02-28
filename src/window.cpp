#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "window.h"

extern HANDLE PHANDLE;
extern ScrollerSizes scroller_sizes;

Window::Window(PHLWINDOW window, double maxy, double box_h, StandardSize width) : window(window), selected(false), width(width)
{
    StandardSize h = scroller_sizes.get_window_default_height(window);
    window->m_vPosition.y = maxy;
    update_height(h, box_h);
    auto deco = makeUnique<SelectionBorders>(this);
    decoration = deco.get();
    HyprlandAPI::addWindowDecoration(PHANDLE, window, std::move(deco));
}

void Window::update_height(StandardSize h, double max)
{
    height = h;
    switch (height) {
    case StandardSize::One:
        box_h = max;
        break;
    case StandardSize::SevenEighths:
        box_h = 7.0 * max / 8.0;
        break;
    case StandardSize::FiveSixths:
        box_h = 5.0 * max / 6.0;
        break;
    case StandardSize::ThreeQuarters:
        box_h = 3.0 * max / 4.0;
        break;
    case StandardSize::TwoThirds:
        box_h = 2.0 * max / 3.0;
        break;
    case StandardSize::FiveEighths:
        box_h = 5.0 * max / 8.0;
        break;
    case StandardSize::OneHalf:
        box_h = 0.5 * max;
        break;
    case StandardSize::ThreeEighths:
        box_h = 3.0 * max / 8.0;
        break;
    case StandardSize::OneThird:
        box_h = max / 3.0;
        break;
    case StandardSize::OneFourth:
        box_h = max / 4.0;
        break;
    case StandardSize::OneSixth:
        box_h = max / 6.0;
        break;
    case StandardSize::OneEighth:
        box_h = max / 8.0;
        break;
    default:
        break;
    }
}

CGradientValueData Window::get_border_color() const
{
    static auto *const *SELECTEDCOL = (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:col.selection_border")->getDataStaticPtr();
    static CHyprColor selected_col = **SELECTEDCOL;
    return selected ? selected_col : window->m_cRealBorderColor;
}

