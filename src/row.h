#ifndef SCROLLER_ROW_H
#define SCROLLER_ROW_H

#include "column.h"

class Row {
public:
    Row(WORKSPACEID workspace);
    ~Row();
    size_t size() const {
        return columns.size();
    }
    WORKSPACEID get_workspace() const { return workspace; }
    const Box &get_max() const { return max; }
    bool has_window(PHLWINDOW window) const {
        for (auto col = columns.first(); col != nullptr; col = col->next()) {
            if (col->data()->has_window(window))
                return true;
        }
        return false;
    }
    const Column *get_active_column() const {
        return active != nullptr? active->data() : nullptr;
    }
    PHLWINDOW get_active_window() const {
        return active != nullptr? active->data()->get_active_window() : nullptr;
    }
    bool is_active(PHLWINDOW window) const {
        return get_active_window() == window;
    }
    void get_windows(std::vector<PHLWINDOWREF> &windows) {
        for (auto col = columns.first(); col != nullptr; col = col->next()) {
            col->data()->get_windows(windows);
        }
    }
    void add_active_window(PHLWINDOW window);
    // Remove a window and re-adapt rows and columns, returning
    // true if successful, or false if this is the last row
    // so the layout can remove it.
    bool remove_window(PHLWINDOW window);
    void focus_window(PHLWINDOW window); 
    bool move_focus(Direction dir, bool focus_wrap);

    void resize_active_column(int step);
    void size_active_column(const std::string &arg);
    void resize_active_window(const Vector2D &delta);
    void set_mode(Mode m, bool silent = false);
    Mode get_mode() const;
    void set_mode_modifier(const ModeModifier &m);
    ModeModifier get_mode_modifier() const;
    void find_auto_insert_point(Mode &new_mode, ListNode<Column *> *&new_active);
    void align_column(Direction dir);
    void pin();
    Column *get_pinned_column() const;
    void selection_toggle();
    void selection_set(PHLWINDOWREF window);
    void selection_all();
    void selection_reset();
    void selection_move(const List<Column *> &columns, Direction direction);
    void selection_get(const Row *row, List<Column *> &selection);
    bool selection_exists() const;
    void move_active_window_to_group(const std::string &name);
    void move_active_column(Direction dir);
    void move_active_window(Direction dir);
    void admit_window(AdmitExpelDirection dir);
    void expel_window(AdmitExpelDirection dir);
    Vector2D predict_window_size() const;
    void post_event(const std::string &event);
    // Returns the old viewport
    bool update_sizes(PHLMONITOR monitor);
    void set_fullscreen_mode_windows(eFullscreenMode mode);
    void set_fullscreen_mode(PHLWINDOW window, eFullscreenMode cur_mode, eFullscreenMode new_mode);
    void fit_size(FitSize fitsize);
    bool is_overview() const;
    void toggle_overview();
    void update_windows(const Box &oldmax, bool force);
    void recalculate_row_geometry();

    void scroll_update(Direction dir, const Vector2D &delta);
    void scroll_end(Direction dir);

private:
    bool move_focus_left(bool focus_wrap); 
    bool move_focus_right(bool focus_wrap);
    void move_focus_begin();
    void move_focus_end();
    void center_active_column();
    // Calculate lateral gaps for a column
    Vector2D calculate_gap_x(const ListNode<Column *> *column) const;
    // Adjust all the columns in the row using 'column' as anchor
    void adjust_columns(ListNode<Column *> *column);
    // Adjust all the columns in the overview
    void adjust_overview_columns();
    void size_active_column(StandardSize size);
    ListNode<Column *> *get_mouse_column() const;

    WORKSPACEID workspace;
    Box full;
    Box max;
    bool overview;
    eFullscreenMode preoverview_fsmode;
    int gap;
    Reorder reorder;
    Mode mode;
    ModeModifier modifier;
    ListNode<Column *> *pinned;
    ListNode<Column *> *active;
    List<Column *> columns;
};

#endif // SCROLLER_ROW_H
