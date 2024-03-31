//#define COLORS_IPC

#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/helpers/Vector2D.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#ifdef COLORS_IPC
#include <hyprland/src/managers/EventManager.hpp>
#endif

#include "scroller.h"

extern HANDLE PHANDLE;

struct Box {
    Box() : x(0), y(0), w(0), h(0) {}
    Box(double x_, double y_, double w_, double h_)
        : x(x_), y(y_), w(w_), h(h_) {}
    Box(Vector2D pos, Vector2D size)
        : x(pos.x), y(pos.y), w(size.x), h(size.y) {}
    Box(const Box &box)
        : x(box.x), y(box.y), w(box.w), h(box.h) {}

    void set_size(double w_, double h_) {
        w = w_;
        h = h_;
    }
    void move(double dx, double dy) {
        x += dx;
        y += dy;
    }
    void set_pos(double x_, double y_) {
        x = x_;
        y = y_;
    }

    double x, y, w, h;
};

enum class ColumnWidth {
    OneThird = 0,
    OneHalf,
    TwoThirds,
    Number,
    Free
};

enum class WindowHeight {
    Auto,
    Free
};

class Window {
public:
    Window(CWindow *window, double box_h) : window(window), box_h(box_h) {}
    CWindow *ptr() { return window; }
    double get_geom_h() const { return box_h; }
    void set_geom_h(double geom_h) { box_h = geom_h; }
private:
    CWindow *window;
    double box_h;
};

class Column {
public:
    Column(CWindow *cwindow, double maxw, double maxh)
        : width(ColumnWidth::OneHalf), height(WindowHeight::Auto), initialized(false), maxdim(false) {
            Window *window = new Window(cwindow, maxh);
            update_width(width, maxw, maxh);
            geom.h = maxh;
            windows.push_back(window);
            active = windows.first();
    }
    Column(Window *window, ColumnWidth width, double maxw, double maxh)
        : width(width), height(WindowHeight::Auto), initialized(true), maxdim(false) {
            window->set_geom_h(maxh);
            update_width(width, maxw, maxh);
            geom.h = maxh;
            windows.push_back(window);
            active = windows.first();
    }
    ~Column() {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            delete win->data();
        }
        windows.clear();
    }
    bool get_init() const { return initialized; }
    void set_init() { initialized = true; }
    size_t size() {
        return windows.size();
    }
    bool has_window(CWindow *window) const {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            if (win->data()->ptr() == window)
                return true;
        }
        return false;
    }
    void remove_window(CWindow *window) {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            if (win->data()->ptr() == window) {
                if (window == active->data()->ptr()) {
                    // Make next window active (like PaperWM)
                    // If it is the last, make the previous one active.
                    // If it is the only window. active will point to nullptr,
                    // but it doesn't matter because the caller will delete
                    // the column.
                    active = active != windows.last() ? active->next() : active->prev();
                }
                windows.erase(win);
                if (size() > 0) {
                    if (height == WindowHeight::Auto) {
                        reset_heights();
                    }
                }
                return;
            }
        }
    }
    void focus_window(CWindow *window) {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            if (win ->data()->ptr() == window) {
                active = win;
                return;
            }
        }
    }
    double get_geom_x() const {
        return geom.x;
    }
    double get_geom_w() const {
        return geom.w;
    }
    bool toggle_fullscreen(const Box &fullbbox) {
        CWindow *wactive = active->data()->ptr();
        wactive->m_bIsFullscreen = !wactive->m_bIsFullscreen;
        if (wactive->m_bIsFullscreen) {
            full = fullbbox;
        }
        return wactive->m_bIsFullscreen;
    }
    // Sets fullscreen even if the active window is not full screen
    // Used in recalculateMonitor
    void set_fullscreen(const Box &fullbbox) {
        // Leave it like this (without enabling full screen in the window).
        // If this is called, it won't work unless the window is also set to full screen
        full = fullbbox;
    }
    void toggle_maximized(double maxw, double maxh) {
        maxdim = !maxdim;
        if (maxdim) {
            maxmem = Vector2D(geom.w, active->data()->get_geom_h());
            geom.w = maxw;
            active->data()->set_geom_h(maxh);
        } else {
            geom.w = maxmem.x;
            active->data()->set_geom_h(maxmem.y);
        }
    }
    bool fullscreen() const {
        return active->data()->ptr()->m_bIsFullscreen;
    }
    bool maximized() const {
        return maxdim;
    }
    // Used by auto-centering of columns
    void set_geom_pos(double x, double y) {
        geom.set_pos(x, y);

    }
    // Recalculates the geometry of the windows in the column
    // The vertical sizes should be correct and updated in
    // toggleheight (for Auto and Free), resizewindow (for Free), and resetheight
    // (for Auto and Free).
    // Also, when creating a new column, it is set to Auto
    // These three dispatchers call set_height(WindowHeight) where the resize happens.
    // The position of the active window will need to be inside the monitor.
    void recalculate_col_geometry(const Vector2D &gap_x, double gap) {
        if (fullscreen()) {
            active->data()->ptr()->m_vPosition = Vector2D(full.x, full.y);
            active->data()->ptr()->m_vSize = Vector2D(full.w, full.h);
            active->data()->ptr()->m_vRealPosition = active->data()->ptr()->m_vPosition;
            active->data()->ptr()->m_vRealSize = active->data()->ptr()->m_vSize;
        } else {
            // In theory, every window in the Columm should have the same size,
            // but the standard layouts don't follow this rule (to make the code
            // simpler?). Windows close to the border of the monitor will have
            // their sizes affected by gaps_out vs. gaps_in.
            // I follow the same rules.
            // Each window has a gap to its bounding box of "gaps_in + border",
            // except on the monitor sides, where the gap is "gaps_out + border",
            // but the window sizes are different because of those different
            // gaps. So the distance between two window border boundaries is
            // two times gaps_in (one per window).

            // The active window needs to be centered or at the top or bottom
            // 1. active visible
            auto above_height = 0.0;
            for (auto win = windows.first(); win != active; win = win->next()) {
                above_height += win->data()->get_geom_h();
            }
            auto below_height = 0.0;
            for (auto win = active->next(); win != nullptr; win = win->next()) {
                below_height += win->data()->get_geom_h();
            }
            bool auto_rows = true;
            if (auto_rows) {
                Window *wactive = active->data();
                CWindow *win = wactive->ptr();
                auto gap0 = active == windows.first() ? 0.0 : gap;
                auto gap1 = active == windows.last() ? 0.0 : gap;
                auto border = win->getRealBorderSize();
                if (above_height + wactive->get_geom_h() + below_height <= geom.h) {
                    // everything fits in the viewport
                    auto padding = 0.5 * (geom.h - above_height -
                                          wactive->get_geom_h() - below_height);
                    win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + gap0 + border + padding + above_height);
                } else {
                    // we consider active as the anchor, and only move it if needed
                    auto above_edge = geom.h - wactive->get_geom_h() - above_height;
                    auto below_edge = geom.h - wactive->get_geom_h() - below_height;
                    if (win->m_vPosition.y - border < geom.y) {
                        // a) if active starts outside above, move to top of
                        // viewport
                        win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + border + gap0);
                    } else if (win->m_vPosition.y - border - gap0 + wactive->get_geom_h() >
                               geom.y + geom.h) {
                        // b) active overflows below, move to bottom of viewport
                        win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + geom.h - gap1 - wactive->get_geom_h() + border + gap0);
                    } else if (above_edge >= 0) {
                        // c) above windows have space to fit together with the
                        // active one (active is not overflowing to the bottom,
                        // already checked in b))
                        win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + above_height + gap0 + border);
                    } else if (below_edge >= 0) {
                        // d) below windows have space to fit together with the
                        // active one (active is not overflowing to the top,
                        // already checked in a))
                        win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + geom.h - below_height - wactive->get_geom_h() + border + gap0);
                    } else {
                        // the window is in a correct position
                        win->m_vPosition = Vector2D(geom.x + border + gap_x.x, geom.y + border + gap0);
                    }
                }
            }
            // 2. adjust positions of windows above
            for (auto w = active->prev(), p = active; w != nullptr; w = w->prev(), p = p->prev()) {
                CWindow *ww = w->data()->ptr();
                CWindow *pw = p->data()->ptr();
                auto wgap0 = w == windows.first() ? 0.0 : gap;
                auto pgap0 = p == windows.first() ? 0.0 : gap;
                auto wborder = ww->getRealBorderSize();
                auto pborder = pw->getRealBorderSize();
                ww->m_vPosition = Vector2D(geom.x + wborder + gap_x.x, pw->m_vPosition.y - pgap0 - pborder - w->data()->get_geom_h() + wborder + wgap0);
            }
            // 3. adjust positions of windows below
            for (auto w = active->next(), p = active; w != nullptr; p = w, w = w->next()) {
                CWindow *ww = w->data()->ptr();
                CWindow *pw = p->data()->ptr();
                auto pgap0 = p == windows.first() ? 0.0 : gap;
                auto wgap0 = w == windows.first() ? 0.0 : gap;
                auto wborder = ww->getRealBorderSize();
                auto pborder = pw->getRealBorderSize();
                ww->m_vPosition = Vector2D(geom.x + wborder + gap_x.x, pw->m_vPosition.y - pborder - pgap0 + p->data()->get_geom_h() + wborder + wgap0);
            }
            for (auto w = windows.first(); w != nullptr; w = w->next()) {
                CWindow *win = w->data()->ptr();
                auto gap0 = w == windows.first() ? 0.0 : gap;
                auto gap1 = w == windows.last() ? 0.0 : gap;
                auto border = win->getRealBorderSize();
                auto wh = w->data()->get_geom_h();
                win->m_vSize = Vector2D(geom.w - 2.0 * border - gap_x.x - gap_x.y, wh - 2.0 * border - gap0 - gap1);
                win->m_vRealPosition = win->m_vPosition;
                win->m_vRealSize = win->m_vSize;
            }
        }
    }
    CWindow *get_active_window() {
        return active->data()->ptr();
    }
    void move_active_up() {
        if (active == windows.first())
            return;

        auto prev = active->prev();
        windows.swap(active, prev);
    }
    void move_active_down() {
        if (active == windows.last())
            return;

        auto next = active->next();
        windows.swap(active, next);
    }
    bool move_focus_up() {
        if (active == windows.first()) {
            // use default dispatch for movefocus (change monitor)
            g_pKeybindManager->m_mDispatchers["movefocus"]("u");
            return false;
        }
        active = active->prev();
        return true;
    }
    bool move_focus_down() {
        if (active == windows.last()) {
            // use default dispatch for movefocus (change monitor)
            g_pKeybindManager->m_mDispatchers["movefocus"]("d");
            return false;
        } else {
            active = active->next();
            return true;
        }
    }
    void admit_window(Window *window, double gap, double border) {
        active = windows.emplace_after(active, window);
        if (height == WindowHeight::Auto) {
            reset_heights();
        }
    }

    Window *expel_active(double gap, double border, double geom_h) {
        Window *window = active->data();
        window->set_geom_h(geom_h);
        auto act = active == windows.first() ? active->next() : active->prev();
        windows.erase(active);
        active = act;
        // recalculate heights
        if (height == WindowHeight::Auto) {
            reset_heights();
        }
        return window;
    }

    ColumnWidth get_width() const {
        return width;
    }
#ifdef COLORS_IPC
    // For IPC events
    std::string get_width_name() const {
        switch (width) {
        case ColumnWidth::OneThird:
            return "OneThird";
        case ColumnWidth::OneHalf:
            return "OneHalf";
        case ColumnWidth::TwoThirds:
            return "TwoThirds";
        case ColumnWidth::Free:
            return "Free";
        default:
            return "";
        }
    }
    std::string get_height_name() const {
        switch (height) {
        case WindowHeight::Auto:
            return "Auto";
        case WindowHeight::Free:
            return "Free";
        default:
            return "";
        }
    }
#endif
    void update_width(ColumnWidth cwidth, double maxw, double maxh) {
        if (maximized()) {
            geom.w = maxw;
        } else {
            switch (cwidth) {
            case ColumnWidth::OneThird:
                geom.w = maxw / 3.0;
                break;
            case ColumnWidth::OneHalf:
                geom.w = maxw / 2.0;
                break;
            case ColumnWidth::TwoThirds:
                geom.w = 2.0 * maxw / 3.0;
                break;
            case ColumnWidth::Free:
                // Only used when creating a column from an expelled window
                geom.w = maxw;
            default:
                break;
            }
        }
        geom.h = maxh;
        width = cwidth;
    }
    // Resets the heights to fit the monitor, each window having the same height.
    // It saves the new height as the "free" one, but doesn't change mode to Free.
    void reset_height() {
        reset_heights();
    }
    void toggle_height() {
        if (height == WindowHeight::Auto) {
            height = WindowHeight::Free;
        } else {
            height = WindowHeight::Auto;
            reset_heights();
        }
    }
    void resize_active_window(double maxw, const Vector2D &gap_x, double gap, double border, const Vector2D &delta) {
        // First, check if resize is possible or it would leave any window
        // with an invalid size.

        // Width check
        auto rwidth = geom.w + delta.x - 2.0 * border - gap_x.x - gap_x.y;
        // Now we check for a size smaller than the maximum possible gap, so
        // we never get in trouble when a window gets expelled from a column
        // with gaps_out, gaps_in, to a column with gaps_in on both sides.
        auto mwidth = geom.w + delta.x - 2.0 * (border + std::max(std::max(gap_x.x, gap_x.y), gap));
         if (mwidth <= 0.0 || rwidth >= maxw)
            return;
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            auto wh = win->data()->ptr()->m_vSize.y;
            if (win == active)
                wh += delta.y;
            else if (height == WindowHeight::Auto) {
                // In Auto, all the windows need to fit the column, so the
                // resizing pushes the active window against the others
                wh -= delta.y / (size() - 1);
            }
            if (wh <= 0.0 || wh > geom.h - 2.0 * border)
                // geom.h already includes gaps_out
                return;
        }
        // Now, resize.
        width = ColumnWidth::Free;

        geom.w += delta.x;
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            Window *window = win->data();
            if (win == active)
                window->set_geom_h(window->get_geom_h() + delta.y);
            else if (height == WindowHeight::Auto) {
                window->set_geom_h(window->get_geom_h() - delta.y / (size() - 1));
            }
        }
    }


private:
    void reset_heights() {
        for (auto w = windows.first(); w != nullptr; w = w->next()) {
            Window *win = w->data();
            win->set_geom_h(geom.h / size());
        }
    }

    ColumnWidth width;
    WindowHeight height;
    bool initialized;
    Box geom;        // bbox of column
    bool maxdim;     // maximized?
    Vector2D maxmem; // keeps a memory of the active window size while in maximized mode
    Box full;        // full screen geometry
    ListNode<Window *> *active;
    List<Window *> windows;
};

class Row {
public:
    Row(CWindow *window)
    : workspace(window->m_iWorkspaceID), active(nullptr) {
        // for gaps outer
        static auto PGAPSIN = CConfigValue<Hyprlang::INT>("general:gaps_in");
        static auto PGAPSOUT = CConfigValue<Hyprlang::INT>("general:gaps_out");
        static auto PBORDERSIZE = CConfigValue<Hyprlang::INT>("general:border_size");

        const auto PMONITOR = g_pCompositor->getMonitorFromID(window->m_iMonitorID);
        const auto SIZE = PMONITOR->vecSize;
        const auto POS = PMONITOR->vecPosition;
        const auto TOPLEFT = PMONITOR->vecReservedTopLeft;
        const auto BOTTOMRIGHT = PMONITOR->vecReservedBottomRight;
        auto fullsz = Box(POS, SIZE);
        auto maxsz = Box(POS.x + TOPLEFT.x + *PGAPSOUT,
                POS.y + TOPLEFT.y + *PGAPSOUT,
                SIZE.x - TOPLEFT.x - BOTTOMRIGHT.x - 2 * *PGAPSOUT,
                SIZE.y - TOPLEFT.y - BOTTOMRIGHT.y - 2 * *PGAPSOUT);

        update_sizes(fullsz, maxsz, *PGAPSIN, *PBORDERSIZE);
    }
    ~Row() {
        for (auto col = columns.first(); col != nullptr; col = col->next()) {
            delete col->data();
        }
        columns.clear();
    }
    int get_workspace() const { return workspace; }
    bool has_window(CWindow *window) const {
        for (auto col = columns.first(); col != nullptr; col = col->next()) {
            if (col->data()->has_window(window))
                return true;
        }
        return false;
    }
    CWindow *get_active_window() const {
        return active->data()->get_active_window();
    }
    bool is_active(CWindow *window) const {
        return get_active_window() == window;
    }
    void add_active_window(CWindow *window) {
        active = columns.emplace_after(active, new Column(window, max.w, max.h));
        recalculate_row_geometry();
    }

    // Remove a window and re-adapt rows and columns, returning
    // true if successful, or false if this is the last row
    // so the layout can remove it.
    bool remove_window(CWindow *window) {
        for (auto c = columns.first(); c != nullptr; c = c->next()) {
            Column *col = c->data();
            if (col->has_window(window)) {
                col->remove_window(window);
                if (col->size() == 0) {
                    if (c == active) {
                        // make NEXT one active before deleting (like PaperWM)
                        // If active was the only one left, doesn't matter
                        // whether it points to end() or not, the row will
                        // be deleted by the parent.
                        active = active != columns.last() ? active->next() : active->prev();
                    }
                    delete col;
                    columns.erase(c);
                    if (columns.empty()) {
                        return false;
                    } else {
                        recalculate_row_geometry();
                        return true;
                    }
                } else {
                    c->data()->recalculate_col_geometry(calculate_gap_x(c), gap);
                    return true;
                }
            }
        }
        return true;
    }
    void focus_window(CWindow *window) {
        for (auto c = columns.first(); c != nullptr; c = c->next()) {
            if (c->data()->has_window(window)) {
                c->data()->focus_window(window);
                active = c;
                recalculate_row_geometry();
                return;
            }
        }
    }
    bool move_focus(Direction dir) {
        switch (dir) {
        case Direction::Left:
            if (!move_focus_left())
                return false;
            break;
        case Direction::Right:
            if (!move_focus_right())
                return false;
            break;
        case Direction::Up:
            if (!active->data()->move_focus_up())
                return false;
            break;
        case Direction::Down:
            if (!active->data()->move_focus_down())
                return false;
            break;
        case Direction::Begin:
            move_focus_begin();
            break;
        case Direction::End:
            move_focus_end();
            break;
        case Direction::Center:
            return true;
        }
        recalculate_row_geometry();
        return true;
    }

private:
    bool move_focus_left() {
        if (active == columns.first()) {
            g_pKeybindManager->m_mDispatchers["movefocus"]("l");
            return false;
        }
        active = active->prev();
        return true;
    }
    bool move_focus_right() {
        if (active == columns.last()) {
            g_pKeybindManager->m_mDispatchers["movefocus"]("r");
            return false;
        }
        active = active->next();
        return true;
    }
    void move_focus_begin() {
        active = columns.first();
    }
    void move_focus_end() {
        active = columns.last();
    }

    // Calculate lateral gaps for a column
    Vector2D calculate_gap_x(const ListNode<Column *> *column) const {
        // First and last columns need a different gap
        auto gap0 = column == columns.first() ? 0.0 : gap;
        auto gap1 = column == columns.last() ? 0.0 : gap;
        return Vector2D(gap0, gap1);
    }

public:
    void resize_active_column(int step) {
        if (active->data()->maximized())
            return;

        ColumnWidth width = active->data()->get_width();
        if (width == ColumnWidth::Free) {
            // When cycle-resizing from Free mode, always move back to default
            // (OneHalf)
            width = ColumnWidth::OneHalf;
        } else {
            int number = static_cast<int>(ColumnWidth::Number);
            width =static_cast<ColumnWidth>(
                    (number + static_cast<int>(width) + step) % number);
        }
        active->data()->update_width(width, max.w, max.h);
        recalculate_row_geometry();
    }
    void resize_active_window(const Vector2D &delta) {
        // If the active window in the active column is fullscreen, ignore.
        if (active->data()->maximized() ||
            active->data()->fullscreen())
            return;

        active->data()->resize_active_window(max.w, calculate_gap_x(active), gap, border, delta);
        recalculate_row_geometry();
    }
    void reset_height() {
        active->data()->reset_height();
        active->data()->recalculate_col_geometry(calculate_gap_x(active), gap);
    }
    void toggle_height() {
        active->data()->toggle_height();
        active->data()->recalculate_col_geometry(calculate_gap_x(active), gap);
    }
    void align_column(Direction dir) {
        if (active->data()->maximized() ||
            active->data()->fullscreen())
            return;

        switch (dir) {
        case Direction::Left:
            active->data()->set_geom_pos(max.x, max.y);
            break;
        case Direction::Right:
            active->data()->set_geom_pos(max.x + max.w - active->data()->get_geom_w(), max.y);
            break;
        case Direction::Center:
            center_active_column();
            break;
        case Direction::Up:
        case Direction::Down:
        case Direction::Begin:
        case Direction::End:
            return;
        }

        recalculate_row_geometry();
    }
private:
    void center_active_column() {
        Column *column = active->data();
        if (column->maximized())
            return;

        switch (column->get_width()) {
        case ColumnWidth::OneThird:
            column->set_geom_pos(max.x + max.w / 3.0, max.y);
            break;
        case ColumnWidth::OneHalf:
            column->set_geom_pos(max.x + max.w / 4.0, max.y);
            break;
        case ColumnWidth::TwoThirds:
            column->set_geom_pos(max.x + max.w / 6.0, max.y);
            break;
        case ColumnWidth::Free:
            column->set_geom_pos(0.5 * (max.w - column->get_geom_w()), max.y);
            break;
        default:
            break;
        }
    }
public:
    void move_active_column(Direction dir) {
        switch (dir) {
        case Direction::Right:
            if (active != columns.last()) {
                auto next = active->next();
                columns.swap(active, next);
            }
            break;
        case Direction::Left:
            if (active != columns.first()) {
                auto prev = active->prev();
                columns.swap(active, prev);
            }
            break;
        case Direction::Up:
            active->data()->move_active_up();
            break;
        case Direction::Down:
            active->data()->move_active_down();
            break;
        case Direction::Begin: {
            if (active == columns.first())
                break;
            columns.move_before(columns.first(), active);
            break;
        }
        case Direction::End: {
            if (active == columns.last())
                break;
            columns.move_after(columns.last(), active);
            break;
        }
        case Direction::Center:
            return;
        }

        recalculate_row_geometry();
    }
    void admit_window_left() {
        if (active->data()->maximized() ||
            active->data()->fullscreen())
            return;
        if (active == columns.first())
            return;

        auto w = active->data()->expel_active(gap, border, max.h);
        auto prev = active->prev();
        if (active->data()->size() == 0) {
            columns.erase(active);
        }
        active = prev;
        active->data()->admit_window(w, gap, border);

        recalculate_row_geometry();
    }
    void expel_window_right() {
        if (active->data()->maximized() ||
            active->data()->fullscreen())
            return;
        if (active->data()->size() == 1)
            // nothing to expel
            return;

        auto w = active->data()->expel_active(gap, border, max.h);
        ColumnWidth width = active->data()->get_width();
        // This code inherits the width of the original column. There is a
        // problem with that when the mode is "Free". The new column may have
        // more reserved space for gaps, and the new window in that column
        // end up having negative size --> crash.
        // There are two options:
        // 1. We don't let column resizing make a column smaller than gap
        // 2. We compromise and inherit the ColumnWidth attribute unless it is
        // "Free". In that case, we force OneHalf (the default).
#if 1
        double maxw = width == ColumnWidth::Free ? active->data()->get_geom_w() : max.w;
#else
        double maxw = max.w;
        if (width == ColumnWidth::Free)
            width = ColumnWidth::OneHalf;
#endif
        active = columns.emplace_after(active, new Column(w, width, maxw, max.h));

        recalculate_row_geometry();
    }
    void update_sizes(const Box &full_size, const Box &max_size, int gaps_in, int border_size) {
        full = full_size;
        max = max_size;
        gap = gaps_in;
        border = border_size;
    }
    void set_fullscreen_active_window() {
        active->data()->set_fullscreen(full);
        // Parameters here don't matter
        active->data()->recalculate_col_geometry(calculate_gap_x(active), gap);
    }
    void toggle_fullscreen_active_window() {
        Column *column = active->data();
        const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(
            column->get_active_window()->m_iWorkspaceID);

        auto fullscreen = active->data()->toggle_fullscreen(full);
        PWORKSPACE->m_bHasFullscreenWindow = fullscreen;

        if (fullscreen) {
            PWORKSPACE->m_efFullscreenMode = eFullscreenMode::FULLSCREEN_FULL;
            column->recalculate_col_geometry(calculate_gap_x(active), gap);
        } else {
            recalculate_row_geometry();
        }
    }
    void toggle_maximize_active_column() {
        Column *column = active->data();
        column->toggle_maximized(max.w, max.h);
        recalculate_row_geometry();
    }
    void recalculate_row_geometry() {
        if (active == nullptr)
            return;

        if (active->data()->fullscreen()) {
            active->data()->recalculate_col_geometry(calculate_gap_x(active), gap);
            return;
        }
#ifdef COLORS_IPC
        // Change border color
    	static auto *const FREECOLUMN = (CGradientValueData *) HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:col.freecolumn_border")->data.get();
        static auto *const ACTIVECOL = (CGradientValueData *)g_pConfigManager->getConfigValuePtr("general:col.active_border")->data.get();
        if (active->data()->get_width() == ColumnWidth::Free) {
            active->data()->get_active_window()->m_cRealBorderColor = *FREECOLUMN;
        } else {
            active->data()->get_active_window()->m_cRealBorderColor = *ACTIVECOL;
        }
        g_pEventManager->postEvent(SHyprIPCEvent{"scroller", active->data()->get_width_name() + "," + active->data()->get_height_name()});
#endif
        // Try to avoid moving the active column unless it is out of the screen.
        auto a_w = std::round(active->data()->get_geom_w());
        double a_x;
        if (active->data()->get_init()) {
            a_x = std::round(active->data()->get_geom_x());
        } else {
            // If the column hasn't been initialized yet (newly created window),
            // we know it will be located on the right of active->prev()
            if (active->prev()) {
                // there is a previous one, locate it on its right
                Column *prev = active->prev()->data();
                a_x = std::round(prev->get_geom_x() + prev->get_geom_w());
            } else {
                // first window, locate it at the center
                a_x = max.x + 0.5 * (max.w - a_w);
            }
            // mark column as initialized
            active->data()->set_init();
        }
        if (a_x < max.x) {
            // active starts outside on the left
            // set it on the left edge
            active->data()->set_geom_pos(max.x, max.y);
        } else if (a_x + a_w > max.x + max.w) {
            // active overflows to the right, move to end of viewport
            active->data()->set_geom_pos(max.x + max.w - a_w, max.y);
        } else {
            // the window is in a correct position
            active->data()->set_geom_pos(a_x, max.y);
        }

        // Adjust the positions of the columns to the left
        for (auto col = active->prev(), prev = active; col != nullptr; prev = col, col = col->prev()) {
            col->data()->set_geom_pos(prev->data()->get_geom_x() - col->data()->get_geom_w(), max.y);
        }
        // Adjust the positions of the columns to the right
        for (auto col = active->next(), prev = active; col != nullptr; prev = col, col = col->next()) {
            col->data()->set_geom_pos(prev->data()->get_geom_x() + prev->data()->get_geom_w(), max.y);
        }

        // Apply column geometry
        for (auto col = columns.first(); col != nullptr; col = col->next()) {
            // First and last columns need a different gap
            auto gap0 = col == columns.first() ? 0.0 : gap;
            auto gap1 = col == columns.last() ? 0.0 : gap;
            col->data()->recalculate_col_geometry(Vector2D(gap0, gap1), gap);
        }
    }

private:
    int workspace;
    Box full;
    Box max;
    int gap;
    int border;
    ListNode<Column *> *active;
    List<Column *> columns;
};


Row *ScrollerLayout::getRowForWorkspace(int workspace) {
    for (auto row = rows.first(); row != nullptr; row = row->next()) {
        if (row->data()->get_workspace() == workspace)
            return row->data();
    }
    return nullptr;
}

Row *ScrollerLayout::getRowForWindow(CWindow *window) {
    for (auto row = rows.first(); row != nullptr; row = row->next()) {
        if (row->data()->has_window(window))
            return row->data();
    }
    return nullptr;
}

/*
    Called when a window is created (mapped)
    The layout HAS TO set the goal pos and size (anim mgr will use it)
    If !animationinprogress, then the anim mgr will not apply an anim.
*/
void ScrollerLayout::onWindowCreatedTiling(CWindow *window, eDirection)
{
    auto s = getRowForWorkspace(window->m_iWorkspaceID);
    if (s == nullptr) {
        s = new Row(window);
        rows.push_back(s);
    }
    s->add_active_window(window);
}

/*
    Called when a window is removed (unmapped)
*/
void ScrollerLayout::onWindowRemovedTiling(CWindow *window)
{
    auto s = getRowForWindow(window);
    if (s == nullptr) {
        return;
    }
    if (!s->remove_window(window)) {
        // It was the last one, remove the row
        for (auto row = rows.first(); row != nullptr; row = row->next()) {
            if (row->data() == s) {
                rows.erase(row);
                delete row->data();
                return;
            }
        }
    }
}

/*
    Internal: called when window focus changes
*/
void ScrollerLayout::onWindowFocusChange(CWindow *window)
{
    if (window == nullptr) { // no window has focus
        return;
    }

    auto s = getRowForWindow(window);
    if (s == nullptr) {
        return;
    }
    s->focus_window(window);
}

/*
    Return tiled status
*/
bool ScrollerLayout::isWindowTiled(CWindow *window)
{
    return getRowForWindow(window) != nullptr;
}

/*
    Called when the monitor requires a layout recalculation
    this usually means reserved area changes
*/
void ScrollerLayout::recalculateMonitor(const int &monitor_id)
{
    auto PMONITOR = g_pCompositor->getMonitorFromID(monitor_id);
    if (!PMONITOR)
        return;
    auto PWORKSPACE = g_pCompositor->getWorkspaceByID(PMONITOR->activeWorkspace);
    if (!PWORKSPACE)
        return;

    g_pHyprRenderer->damageMonitor(PMONITOR);

    auto s = getRowForWorkspace(PWORKSPACE->m_iID);
    if (s == nullptr)
        return;

    // for gaps outer
    static auto PGAPSIN       = CConfigValue<Hyprlang::INT>("general:gaps_in");
    static auto PGAPSOUT       = CConfigValue<Hyprlang::INT>("general:gaps_out");
    static auto PBORDERSIZE       = CConfigValue<Hyprlang::INT>("general:border_size");

    const auto SIZE = PMONITOR->vecSize;
    const auto POS = PMONITOR->vecPosition;
    const auto TOPLEFT = PMONITOR->vecReservedTopLeft;
    const auto BOTTOMRIGHT = PMONITOR->vecReservedBottomRight;
    auto fullsz = Box(POS, SIZE);
    auto maxsz = Box(POS.x + TOPLEFT.x + *PGAPSOUT,
            POS.y + TOPLEFT.y + *PGAPSOUT,
            SIZE.x - TOPLEFT.x - BOTTOMRIGHT.x - 2 * *PGAPSOUT,
            SIZE.y - TOPLEFT.y - BOTTOMRIGHT.y - 2 * *PGAPSOUT);

    s->update_sizes(fullsz, maxsz, *PGAPSIN, *PBORDERSIZE);
    if (PWORKSPACE->m_bHasFullscreenWindow && PWORKSPACE->m_efFullscreenMode == FULLSCREEN_FULL) {
        s->set_fullscreen_active_window();
    } else {
        s->recalculate_row_geometry();
    }
    if (PMONITOR->specialWorkspaceID) {
        auto sw = getRowForWorkspace(PMONITOR->specialWorkspaceID);
        if (sw == nullptr) {
            return;
        }
        sw->update_sizes(fullsz, maxsz, *PGAPSIN, *PBORDERSIZE);
        sw->recalculate_row_geometry();
    }
}

/*
    Called when the compositor requests a window
    to be recalculated, e.g. when pseudo is toggled.
*/
void ScrollerLayout::recalculateWindow(CWindow *window)
{
    auto s = getRowForWindow(window);
    if (s == nullptr)
        return;

    s->recalculate_row_geometry();
}

/*
    Called when a user requests a resize of the current window by a vec
    Vector2D holds pixel values
    Optional pWindow for a specific window
*/
void ScrollerLayout::resizeActiveWindow(const Vector2D &delta,
                                        eRectCorner corner, CWindow *window)
{
    // We support resizing columns and windows.
    // ColumnWidth mode:
    // Changing column mode only affects the width
    // OneHalf->TwoThirds->OneThird->OneHalf
    // Resizing a window changes the column mode to Free.

    // WindowHeight mode:
    // Auto->Free
    // Windows resize:
    // - When in free window mode, they don't need to fit the max column height
    //   but get auto-centered
    // - When in auto window mode, all the windows in the column fit the
    //   column height. There is no need for auto-centering.
    // Resizing a window changes the ColumnWidth and WindowHeight modes to Free
    // dispatches:
    //   resetheight: Sets Auto for column's WindowHeight and re-applies geom.
    //   toggleheight: Toggles WindowHeight (Auto <-> Free).
    //
    // WindowHeight and ColumnWidth are totally independent.
    const auto PWINDOW = window ? window : g_pCompositor->m_pLastWindow;
    auto s = getRowForWindow(PWINDOW);
    if (s == nullptr)
        return;

    s->resize_active_window(delta);
}

/*
   Called when a window / the user requests to toggle the fullscreen state of a
   window. The layout sets all the fullscreen flags. It can either accept or
   ignore.
*/
void ScrollerLayout::fullscreenRequestForWindow(CWindow *window,
                                                eFullscreenMode fullscreenmode,
                                                bool on)
{
    auto s = getRowForWindow(window);
    if (s == nullptr) {
        return;
    } // assuming window is active for now

    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(window->m_iWorkspaceID);
    switch (fullscreenmode) {
        case eFullscreenMode::FULLSCREEN_FULL:
            if (on == window->m_bIsFullscreen)
                return;

            // if the window wants to be fullscreen but there already is one, ignore the request.
            if (PWORKSPACE->m_bHasFullscreenWindow && on)
                return;

            s->toggle_fullscreen_active_window();
            break;
        case eFullscreenMode::FULLSCREEN_MAXIMIZED:
            s->toggle_maximize_active_column();
            break;
        default:
            return;
    }
}

/*
    Called when a dispatcher requests a custom message
    The layout is free to ignore.
    std::any is the reply. Can be empty.
*/
std::any ScrollerLayout::layoutMessage(SLayoutMessageHeader header, std::string content)
{
    return "";
}

/*
    Required to be handled, but may return just SWindowRenderLayoutHints()
    Called when the renderer requests any special draw flags for
    a specific window, e.g. border color for groups.
*/
SWindowRenderLayoutHints ScrollerLayout::requestRenderHints(CWindow *)
{
    return {};
}

/*
    Called when the user requests two windows to be swapped places.
    The layout is free to ignore.
*/
void ScrollerLayout::switchWindows(CWindow *, CWindow *)
{
}

/*
    Called when the user requests a window move in a direction.
    The layout is free to ignore.
*/
void ScrollerLayout::moveWindowTo(CWindow *window, const std::string &direction)
{
    auto s = getRowForWindow(window);
    if (s == nullptr) {
        return;
    } else if (!(s->is_active(window))) {
        // cannot move non active window?
        return;
    }

    switch (direction.at(0)) {
        case 'l': s->move_active_column(Direction::Left); break;
        case 'r': s->move_active_column(Direction::Right); break;
        case 'u': s->move_active_column(Direction::Up); break;
        case 'd': s->move_active_column(Direction::Down); break;
        default: break;
    }
}

/*
    Called when the user requests to change the splitratio by or to X
    on a window
*/
void ScrollerLayout::alterSplitRatio(CWindow *, float, bool)
{
}

/*
    Called when something wants the current layout's name
*/
std::string ScrollerLayout::getLayoutName()
{
    return "scroller";
}

/*
    Called for getting the next candidate for a focus
*/
CWindow* ScrollerLayout::getNextWindowCandidate(CWindow *old_window)
{
    // This is called when a windows in unmapped. This means the window
    // has also been removed from the layout. In that case, returning the
    // new active window is the correct thing.
    int workspace_id = g_pCompositor->m_pLastMonitor->activeWorkspace;
    auto s = getRowForWorkspace(workspace_id);
    if (s == nullptr)
        return nullptr;
    else
        return s->get_active_window();
}

/*
    Called for replacing any data a layout has for a new window
*/
void ScrollerLayout::replaceWindowDataWith(CWindow *from, CWindow *to)
{
}

void ScrollerLayout::onEnable() {
    for (auto& window : g_pCompositor->m_vWindows) {
        if (window->m_bIsFloating || !window->m_bIsMapped || window->isHidden())
            continue;

        onWindowCreatedTiling(window.get());
        recalculateMonitor(window.get()->m_iMonitorID);
    }
}

void ScrollerLayout::onDisable() {
    for (auto row = rows.first(); row != nullptr; row = row->next()) {
        delete row->data();
    }
    rows.clear();
}

/*
    Called to predict the size of a newly opened window to send it a configure.
    Return 0,0 if unpredictable
*/
Vector2D predictSizeForNewWindowTiled() {
    return Vector2D(0.0, 0.0);
}

void ScrollerLayout::cycle_window_size(int workspace, int step)
{
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }

    s->resize_active_column(step);
}

void ScrollerLayout::move_focus(int workspace, Direction direction)
{
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        // if workspace is empty, use the deault movefocus, which now
        // is "move to another monitor" (pass the direction)
        switch (direction) {
            case Direction::Left:
                g_pKeybindManager->m_mDispatchers["movefocus"]("l");
                break;
            case Direction::Right:
                g_pKeybindManager->m_mDispatchers["movefocus"]("r");
                break;
            case Direction::Up:
                g_pKeybindManager->m_mDispatchers["movefocus"]("u");
                break;
            case Direction::Down:
                g_pKeybindManager->m_mDispatchers["movefocus"]("d");
                break;
            default:
                break;
        }
        return;
    }

    if (!s->move_focus(direction)) {
        // changed monitor
        s = getRowForWorkspace(g_pCompositor->m_pLastMonitor->activeWorkspace);
        if (s == nullptr) {
            // monitor is empty
            return;
        }
    }
    g_pCompositor->focusWindow(s->get_active_window());
}

void ScrollerLayout::move_window(int workspace, Direction direction) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }

    s->move_active_column(direction);
    g_pCompositor->focusWindow(s->get_active_window());
}

void ScrollerLayout::align_window(int workspace, Direction direction) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }

    s->align_column(direction);
}

void ScrollerLayout::admit_window_left(int workspace) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }
    s->admit_window_left();
}

void ScrollerLayout::expel_window_right(int workspace) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }
    s->expel_window_right();
}

void ScrollerLayout::reset_height(int workspace) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }
    s->reset_height();
}

void ScrollerLayout::toggle_height(int workspace) {
    auto s = getRowForWorkspace(workspace);
    if (s == nullptr) {
        return;
    }
    s->toggle_height();
}

