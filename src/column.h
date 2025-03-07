#ifndef SCROLLER_COLUMN_H
#define SCROLLER_COLUMN_H

#include "list.h"
#include "common.h"
#include "window.h"

class Row;

class Column {
public:
    Column(PHLWINDOW cwindow, const Row *row);
    Column(Window *window, StandardSize width, double maxw, const Row *row);
    Column(const Row *row, const Column *column, List<Window *> &windows);
    ~Column();
    std::string get_name() const { return name; }
    void set_name (const std::string &str) { name = str; }
    size_t size() const {
        return windows.size();
    }
    void get_windows(std::vector<PHLWINDOWREF> &pWindows) {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            pWindows.push_back(win->data()->get_window());
        }
    }
    bool has_window(PHLWINDOW window) const;
    Window *get_window(PHLWINDOW window) const;
    void add_active_window(PHLWINDOW window);
    void remove_window(PHLWINDOW window);
    void focus_window(PHLWINDOW window);
    double get_geom_x() const {
        return geom.x;
    }
    double get_geom_vy() const {
        return geom.vy;
    }
    double get_geom_w() const {
        return geom.w;
    }
    // Used by Row::fit_width() and Row::toggle_overview()
    void set_geom_w(double w) {
        geom.w = w;
    }
    Vector2D get_height() const {
        double h = windows.first()->data()->get_geom_y(0.0);
        Vector2D height(h, 0.0);
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            h += win->data()->get_geom_h();
        }
        height.y = h;
        return height;
    }
    void scale(const Vector2D &bmin, const Vector2D &start, double scale, double gap) {
        for (auto win = windows.first(); win != nullptr; win = win->next()) {
            auto gap0 = win == windows.first() ? 0.0 : gap;
            auto gap1 = win == windows.last() ? 0.0 : gap;
            win->data()->scale(bmin, start, scale, gap0, gap1);
        }
    }
    void push_overview_geom() {
        mem.geom = geom;
        for (auto w = windows.first(); w != nullptr; w = w->next()) {
            w->data()->push_overview_geom();
        }
    }
    void pop_overview_geom() {
        geom = mem.geom;
        for (auto w = windows.first(); w != nullptr; w = w->next()) {
            w->data()->pop_overview_geom();
        }
    }
    void set_active_window_geometry(const Box &box) {
        active->data()->set_geometry(box);
    }
    bool fullscreen() const {
        return active->data()->fullscreen_state() != eFullscreenMode::FSMODE_NONE;
    }
    bool maximized() const {
        return active->data()->fullscreen_state() == eFullscreenMode::FSMODE_MAXIMIZED;
    }
    // Used by auto-centering of columns
    void set_geom_pos(double x, double vy) {
        geom.x = x;
        geom.vy = vy;
    }
    // Recalculates the geometry of the windows in the column
    void recalculate_col_geometry(const Vector2D &gap_x, double gap, bool animate);
    // Recalculates the geometry of the windows in the column for overview mode
    void recalculate_col_geometry_overview(const Vector2D &gap_x, double gap);
    PHLWINDOW get_active_window() {
        return active->data()->get_window();
    }
    void move_active_up();
    void move_active_down();
    bool move_focus_up(bool focus_wrap);
    bool move_focus_down(bool focus_wrap);
    void admit_window(Window *window);
    Window *expel_active(const Vector2D &gap_x);
    void align_window(Direction direction, const Vector2D &gap_x, double gap);

    StandardSize get_width() const {
        return width;
    }
    // used by Row::fit_width()
    void set_width_free() {
        width = StandardSize::Free;
    }
    // Update heights according to new maxh
    void update_heights();
    void update_width(StandardSize cwidth, double maxw, bool internal_too = true);
    void fit_size(FitSize fitsize, const Vector2D &gap_x, double gap);
    void cycle_size_active_window(int step, const Vector2D &gap_x, double gap);
    void size_active_window(StandardSize height, const Vector2D &gap_x, double gap);
    void resize_active_window(const Vector2D &gap_x, double gap, const Vector2D &delta);
    void selection_toggle();
    void selection_set(PHLWINDOWREF window);
    void selection_all();
    void selection_reset();
    Column *selection_get(const Row *row);
    bool selection_exists() const;
    void pin(bool pin) const;
    void scroll_update(double delta_y);
    void scroll_end(Direction dir, double gap);

private:
    // Adjust all the windows in the column using 'window' as anchor
    void adjust_windows(ListNode<Window *> *win, const Vector2D &gap_x, double gap, bool animate);

    struct ColumnGeom {
        double x;
        double w;
        double vy;    // mini visible y
    };
    struct Memory {
        ColumnGeom geom;        // memory of the column's box while in overview mode
    };
    StandardSize width;
    Reorder reorder;
    ColumnGeom geom; // x location and width of the column
    Memory mem;      // memory
    ListNode<Window *> *active;
    List<Window *> windows;
    std::string name;
    const Row *row;
};



#endif  // SCROLLER_COLUMN_H
