#include <hyprland/src/config/ConfigValue.hpp>

#include "common.h"
#include "functions.h"
#include "row.h"
#include "overview.h"

extern HANDLE PHANDLE;
extern Overview *overviews;
extern std::function<SDispatchResult(std::string)> orig_moveFocusTo;
extern CycleSizes column_widths;
extern ScrollerSizes scroller_sizes;

Row::Row(WORKSPACEID workspace)
    : workspace(workspace), overview(false),
      reorder(Reorder::Auto), pinned(nullptr), active(nullptr)
{
    post_event("overview");
    const auto PMONITOR = g_pCompositor->m_pLastMonitor.lock();
    set_mode(scroller_sizes.get_mode(PMONITOR));
    update_sizes(PMONITOR);
}

Row::~Row()
{
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        delete col->data();
    }
    columns.clear();
}

void Row::add_active_window(PHLWINDOW window)
{
    bool overview_on = overview;
    if (overview)
        toggle_overview();

    eFullscreenMode fsmode;
    if (active != nullptr) {
        auto awindow = get_active_window();
        fsmode = window_fullscreen_state(awindow);
        if (fsmode != eFullscreenMode::FSMODE_NONE) {
            toggle_window_fullscreen_internal(awindow, eFullscreenMode::FSMODE_NONE);
        }
    } else {
        fsmode = eFullscreenMode::FSMODE_NONE;
    }

    if (active && mode == Mode::Column) {
        active->data()->add_active_window(window);
        active->data()->recalculate_col_geometry(calculate_gap_x(active), gap);
    } else {
        active = columns.emplace_after(active, new Column(window, max.w, this));
        reorder = Reorder::Auto;
        recalculate_row_geometry();
    }
    if (fsmode != eFullscreenMode::FSMODE_NONE) {
        toggle_window_fullscreen_internal(window, fsmode);
        force_focus_to_window(window);
    }
    if (overview_on)
        toggle_overview();
}

// Remove a window and re-adapt rows and columns, returning
// true if successful, or false if this is the last row
// so the layout can remove it.
bool Row::remove_window(PHLWINDOW window)
{
    bool overview_on = overview;
    if (overview)
        toggle_overview();

    eFullscreenMode fsmode = window_fullscreen_state(window);
    if (fsmode != eFullscreenMode::FSMODE_NONE) {
        toggle_window_fullscreen_internal(window, eFullscreenMode::FSMODE_NONE);
    }

    reorder = Reorder::Auto;
    for (auto c = columns.first(); c != nullptr; c = c->next()) {
        Column *col = c->data();
        if (col->has_window(window)) {
            col->remove_window(window);
            if (col->size() == 0) {
                if (c == pinned) {
                    pinned = nullptr;
                }
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
                    break;
                }
            } else {
                c->data()->recalculate_col_geometry(calculate_gap_x(c), gap);
                break;
            }
        }
    }
    if (fsmode != eFullscreenMode::FSMODE_NONE) {
        PHLWINDOW awindow = get_active_window();
        toggle_window_fullscreen_internal(awindow, fsmode);
        force_focus_to_window(awindow);
    }
    if (overview_on)
        toggle_overview();

    return true;
}

void Row::focus_window(PHLWINDOW window)
{
    for (auto c = columns.first(); c != nullptr; c = c->next()) {
        if (c->data()->has_window(window)) {
            c->data()->focus_window(window);
            active = c;
            recalculate_row_geometry();
            return;
        }
    }
}

bool Row::move_focus(Direction dir, bool focus_wrap)
{
    bool changed_workspace = false;

    switch (dir) {
    case Direction::Left:
        if (!move_focus_left(focus_wrap))
            changed_workspace = true;
        break;
    case Direction::Right:
        if (!move_focus_right(focus_wrap))
            changed_workspace = true;
        break;
    case Direction::Up:
        if (!active->data()->move_focus_up(focus_wrap))
            changed_workspace = true;
        break;
    case Direction::Down:
        if (!active->data()->move_focus_down(focus_wrap))
            changed_workspace = true;
        break;
    case Direction::Begin:
        move_focus_begin();
        break;
    case Direction::End:
        move_focus_end();
        break;
    default:
        return false;
    }

    reorder = Reorder::Auto;
    recalculate_row_geometry();

    return changed_workspace;
}

bool Row::move_focus_left(bool focus_wrap)
{
    if (active == columns.first()) {
        PHLMONITOR monitor = g_pCompositor->getMonitorInDirection('l');
        if (monitor == nullptr) {
            if (focus_wrap)
                active = columns.last();
            return true;
        }

        orig_moveFocusTo("l");
        return false;
    }
    active = active->prev();
    return true;
}

bool Row::move_focus_right(bool focus_wrap)
{
    if (active == columns.last()) {
        PHLMONITOR monitor = g_pCompositor->getMonitorInDirection('r');
        if (monitor == nullptr) {
            if (focus_wrap)
                active = columns.first();
            return true;
        }

        orig_moveFocusTo("r");
        return false;
    }
    active = active->next();
    return true;
}

void Row::move_focus_begin()
{
    active = columns.first();
}

void Row::move_focus_end()
{
    active = columns.last();
}

// Calculate lateral gaps for a column
Vector2D Row::calculate_gap_x(const ListNode<Column *> *column) const
{
    // First and last columns need a different gap
    auto gap0 = column == columns.first() ? 0.0 : gap;
    auto gap1 = column == columns.last() ? 0.0 : gap;
    return Vector2D(gap0, gap1);
}

void Row::resize_active_column(int step)
{
    if (active->data()->fullscreen())
        return;

    bool overview_on = overview;
    if (overview)
        toggle_overview();

    if (mode == Mode::Column) {
        active->data()->cycle_size_active_window(step, calculate_gap_x(active), gap);
    } else {
        static auto const *column_widths_str = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:column_widths")->getDataStaticPtr();
        column_widths.update(*column_widths_str);

        StandardSize width = active->data()->get_width();
        if (width == StandardSize::Free) {
            // When cycle-resizing from Free mode, always move back to first
            width = column_widths.get_default();
        } else {
            width = column_widths.get_next(width, step);
        }
        active->data()->update_width(width, max.w);
        reorder = Reorder::Auto;
        recalculate_row_geometry();
    }
    if (overview_on)
        toggle_overview();
}

void Row::resize_active_window(const Vector2D &delta)
{
    // If the active window in the active column is fullscreen, ignore.
    if (active->data()->fullscreen())
        return;
    if (overview)
        return;

    active->data()->resize_active_window(calculate_gap_x(active), gap, delta);
    recalculate_row_geometry();
}

void Row::set_mode(Mode m, bool silent)
{
    mode = m;
    if (!silent) {
        post_event("mode");
    }
}

Mode Row::get_mode() const
{
    return mode;
}

void Row::align_column(Direction dir)
{
    if (active->data()->fullscreen())
        return;
    if (overview)
        return;

    switch (dir) {
    case Direction::Left:
        active->data()->set_geom_pos(max.x, max.y);
        break;
    case Direction::Right:
        active->data()->set_geom_pos(max.x + max.w - active->data()->get_geom_w(), max.y);
        break;
    case Direction::Center:
        if (mode == Mode::Column) {
            const Vector2D gap_x = calculate_gap_x(active);
            active->data()->align_window(Direction::Center, gap_x, gap);
            active->data()->recalculate_col_geometry(gap_x, gap);
            return;
        } else {
            center_active_column();
        }
        break;
    case Direction::Up:
    case Direction::Down: {
        const Vector2D gap_x = calculate_gap_x(active);
        active->data()->align_window(dir, gap_x, gap);
        active->data()->recalculate_col_geometry(gap_x, gap);
        return;
    } break;
    default:
        return;
    }
    reorder = Reorder::Lazy;
    recalculate_row_geometry();
}

void Row::pin()
{
    if (pinned != nullptr) {
        pinned = nullptr;
    } else {
        pinned = active;
    }
}

void Row::selection_toggle()
{
    active->data()->selection_toggle();
}

void Row::selection_set(PHLWINDOWREF window)
{
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        col->data()->selection_set(window);
    }
}

void Row::selection_reset()
{
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        col->data()->selection_reset();
    }
}

static void insert_selection_before(List <Column *> &columns, ListNode<Column *> *node, const List<Column *> &selection)
{
    for (auto col = selection.first(); col != nullptr; col = col->next()) {
        columns.insert_before(node, col->data());
    }
}

static void insert_selection_after(List <Column *> &columns, ListNode<Column *> *node, const List<Column *> &selection)
{
    for (auto col = selection.last(); col != nullptr; col = col->prev()) {
        columns.insert_after(node, col->data());
    }
}

void Row::selection_move(const List<Column *> &selection, Direction direction)
{
    if (columns.size() == 0) {
        for (auto col = selection.first(); col != nullptr; col = col->next()) {
            columns.push_back(col->data());
        }
        active = columns.first();
    } else {
        switch (direction) {
        case Direction::Left:
            insert_selection_before(columns, active, selection);
            break;
        case Direction::Begin:
            insert_selection_before(columns, columns.first(), selection);
            break;
        case Direction::End:
            insert_selection_after(columns, columns.last(), selection);
            break;
        case Direction::Right:
        default:
            insert_selection_after(columns, active, selection);
            break;
        }
    }
}

bool Row::selection_exists() const
{
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        if (col->data()->selection_exists())
            return true;
    }
    return false;
}

void Row::selection_get(const Row *row, List<Column *> &selection)
{
    bool overview_on = overview;
    if (overview)
        toggle_overview();

    auto col = columns.first();
    while (col != nullptr) {
        auto next = col->next();
        Column *column = col->data()->selection_get(row);
        if (column != nullptr) {
            selection.push_back(column);
            if (col->data()->size() == 0) {
                // Removed all windows
                if (col == active) {
                    active = active != columns.last() ? active->next() : active->prev();
                }
                columns.erase(col);
                delete col->data();
            }
        }
        col = next;
    }

    if (overview_on)
        toggle_overview();
}

void Row::center_active_column()
{
    Column *column = active->data();
    if (column->fullscreen())
        return;

    switch (column->get_width()) {
    case StandardSize::OneEighth:
        column->set_geom_pos(max.x + 7.0 * max.w / 16.0, max.y);
        break;
    case StandardSize::OneSixth:
        column->set_geom_pos(max.x + 5.0 * max.w / 12.0, max.y);
        break;
    case StandardSize::OneFourth:
        column->set_geom_pos(max.x + 3.0 * max.w / 8.0, max.y);
        break;
    case StandardSize::OneThird:
        column->set_geom_pos(max.x + max.w / 3.0, max.y);
        break;
    case StandardSize::ThreeEighths:
        column->set_geom_pos(max.x + 5.0 * max.w / 16.0, max.y);
        break;
    case StandardSize::OneHalf:
        column->set_geom_pos(max.x + max.w / 4.0, max.y);
        break;
    case StandardSize::FiveEighths:
        column->set_geom_pos(max.x + 3.0 * max.w / 16.0, max.y);
        break;
    case StandardSize::TwoThirds:
        column->set_geom_pos(max.x + max.w / 6.0, max.y);
        break;
    case StandardSize::ThreeQuarters:
        column->set_geom_pos(max.x + max.w / 8.0, max.y);
        break;
    case StandardSize::FiveSixths:
        column->set_geom_pos(max.x + max.w / 12.0, max.y);
        break;
    case StandardSize::SevenEighths:
        column->set_geom_pos(max.x + 1.0 * max.w / 16.0, max.y);
        break;
    case StandardSize::One:
        column->set_geom_pos(max.x, max.y);
        break;
    case StandardSize::Free:
        column->set_geom_pos(0.5 * (max.w - column->get_geom_w()), max.y);
        break;
    default:
        break;
    }
}

void Row::move_active_window_to_group(const std::string &name)
{
    for (auto c = columns.first(); c != nullptr; c = c->next()) {
        Column *col = c->data();
        if (col->get_name() == name) {
            PHLWINDOW window = active->data()->get_active_window();
            remove_window(window);
            col->add_active_window(window);
            if (!window->isFullscreen())
                col->recalculate_col_geometry(calculate_gap_x(c), gap);
            active = c;
            if (!window->isFullscreen())
                recalculate_row_geometry();
            else {
                force_focus_to_window(window);
            }
            return;
        }
    }
    active->data()->set_name(name);
}

void Row::move_active_column(Direction dir)
{
    bool overview_on = overview;
    if (overview)
        toggle_overview();

    auto window = active->data()->get_active_window();
    eFullscreenMode fsmode = window_fullscreen_state(window);
    if (fsmode != eFullscreenMode::FSMODE_NONE) {
        toggle_window_fullscreen_internal(window, eFullscreenMode::FSMODE_NONE);
    }

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
    default:
        return;
    }

    reorder = Reorder::Auto;
    recalculate_row_geometry();
    // Now the columns are in the right order, recalculate again
    recalculate_row_geometry();

    if (fsmode != eFullscreenMode::FSMODE_NONE) {
        window = active->data()->get_active_window();
        toggle_window_fullscreen_internal(window, fsmode);
        force_focus_to_window(window);
    }

    if (overview_on)
        toggle_overview();
}

void Row::admit_window_left()
{
    if (active->data()->fullscreen())
        return;
    if (active == columns.first())
        return;

    bool overview_on = overview;
    if (overview)
        toggle_overview();

    auto w = active->data()->expel_active();
    auto prev = active->prev();
    if (active->data()->size() == 0) {
        if (active == pinned)
            pinned = nullptr;
        columns.erase(active);
    }
    active = prev;
    active->data()->admit_window(w);

    reorder = Reorder::Auto;
    recalculate_row_geometry();

    post_event("admitwindow");

    if (overview_on)
        toggle_overview();
}

void Row::expel_window_right()
{
    if (active->data()->fullscreen())
        return;
    if (active->data()->size() == 1)
        // nothing to expel
        return;

    bool overview_on = overview;
    if (overview)
        toggle_overview();

    auto w = active->data()->expel_active();
    StandardSize width = active->data()->get_width();
    // This code inherits the width of the original column. There is a
    // problem with that when the mode is "Free". The new column may have
    // more reserved space for gaps, and the new window in that column
    // end up having negative size --> crash.
    // There are two options:
    // 1. We don't let column resizing make a column smaller than gap
    // 2. We compromise and inherit the StandardSize attribute unless it is
    // "Free". In that case, we force OneHalf (the default).
#if 1
    double maxw = width == StandardSize::Free ? active->data()->get_geom_w() : max.w;
#else
    double maxw = max.w;
    if (width == StandardSize::Free)
        width = StandardSize::OneHalf;
#endif
    active = columns.emplace_after(active, new Column(w, width, maxw, this));
    // Initialize the position so it is located after its previous column
    // This helps the heuristic in recalculate_row_geometry()
    active->data()->set_geom_pos(active->prev()->data()->get_geom_x() + active->prev()->data()->get_geom_w(), max.y);

    reorder = Reorder::Auto;
    recalculate_row_geometry();

    post_event("expelwindow");

    if (overview_on)
        toggle_overview();
}

Vector2D Row::predict_window_size() const
{
    return Vector2D(0.5 * max.w, max.h);
}

void Row::post_event(const std::string &event)
{
    if (event == "mode") {
        g_pEventManager->postEvent(SHyprIPCEvent{"scroller", std::format("mode, {}", mode == Mode::Row ? "row" : "column")});
    } else if (event == "overview") {
        g_pEventManager->postEvent(SHyprIPCEvent{"scroller", std::format("overview, {}", overview ? 1 : 0)});
    } else if (event == "admitwindow") {
        g_pEventManager->postEvent(SHyprIPCEvent{"scroller", "admitwindow"});
    } else if (event == "expelwindow") {
        g_pEventManager->postEvent(SHyprIPCEvent{"scroller", "expelwindow"});
    }
}

// Returns true/false if columns/windows need to be recalculated
bool Row::update_sizes(PHLMONITOR monitor)
{
    // for gaps outer
    static auto PGAPSINDATA = CConfigValue<Hyprlang::CUSTOMTYPE>("general:gaps_in");
    static auto PGAPSOUTDATA = CConfigValue<Hyprlang::CUSTOMTYPE>("general:gaps_out");
    auto *const PGAPSIN = (CCssGapData *)(PGAPSINDATA.ptr())->getData();
    auto *const PGAPSOUT = (CCssGapData *)(PGAPSOUTDATA.ptr())->getData();
    // For now, support only constant CCssGapData
    auto gaps_in = PGAPSIN->top;
    auto gaps_out = *PGAPSOUT;

    const auto SIZE = monitor->vecSize;
    const auto POS = monitor->vecPosition;
    const auto TOPLEFT = monitor->vecReservedTopLeft;
    const auto BOTTOMRIGHT = monitor->vecReservedBottomRight;

    full = Box(POS, SIZE);
    const Box newmax = Box(POS.x + TOPLEFT.x + gaps_out.left,
                           POS.y + TOPLEFT.y + gaps_out.top,
                           SIZE.x - TOPLEFT.x - BOTTOMRIGHT.x - gaps_out.left - gaps_out.right,
                           SIZE.y - TOPLEFT.y - BOTTOMRIGHT.y - gaps_out.top - gaps_out.bottom);
    bool changed = gap != gaps_in;
    gap = gaps_in;

    if (max != newmax)
        changed = true;

    max = newmax;
    return changed;
}

void Row::set_fullscreen_mode_windows(eFullscreenMode mode)
{
    Column *column = active->data();
    switch (mode) {
    case eFullscreenMode::FSMODE_NONE:
        break;
    case eFullscreenMode::FSMODE_FULLSCREEN:
        column->set_active_window_geometry(full);
        break;
    case eFullscreenMode::FSMODE_MAXIMIZED:
        column->set_active_window_geometry(max);
        break;
    default:
        break;
    }
}

void Row::set_fullscreen_mode(PHLWINDOW window, eFullscreenMode cur_mode, eFullscreenMode new_mode)
{
    reorder = Reorder::Auto;
    Window *win = nullptr;
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        win = col->data()->get_window(window);
        if (win != nullptr)
            break;
    }
    if (win != nullptr) {
        switch (new_mode) {
        case eFullscreenMode::FSMODE_NONE:
            win->pop_fullscreen_geom();
            break;
        case eFullscreenMode::FSMODE_FULLSCREEN:
            if (cur_mode == eFullscreenMode::FSMODE_NONE)
                win->push_fullscreen_geom();
            win->set_geometry(full);
            break;
        case eFullscreenMode::FSMODE_MAXIMIZED:
            if (cur_mode == eFullscreenMode::FSMODE_NONE)
                win->push_fullscreen_geom();
            win->set_geometry(max);
            break;
        default:
            return;
        }
    }
}

void Row::fit_size(FitSize fitsize)
{
    if (active->data()->fullscreen()) {
        return;
    }
    if (overview) {
        return;
    }
    if (mode == Mode::Column) {
        active->data()->fit_size(fitsize, calculate_gap_x(active), gap);
        return;
    }
    ListNode<Column *> *from, *to;
    switch (fitsize) {
    case FitSize::Active:
        from = to = active;
        break;
    case FitSize::Visible:
        for (auto c = columns.first(); c != nullptr; c = c->next()) {
            Column *col = c->data();
            auto c0 = col->get_geom_x();
            auto c1 = std::round(col->get_geom_x() + col->get_geom_w());
            if ((c0 < max.x + max.w && c0 >= max.x) ||
                (c1 > max.x && c1 <= max.x + max.w) ||
                // should never happen as columns are never wider than the screen
                (c0 < max.x && c1 >= max.x + max.w)) {
                from = c;
                break;
            }
        }
        for (auto c = columns.last(); c != nullptr; c = c->prev()) {
            Column *col = c->data();
            auto c0 = col->get_geom_x();
            auto c1 = std::round(col->get_geom_x() + col->get_geom_w());
            if ((c0 < max.x + max.w && c0 >= max.x) ||
                (c1 > max.x && c1 <= max.x + max.w) ||
                // should never happen as columns are never wider than the screen
                (c0 < max.x && c1 >= max.x + max.w)) {
                to = c;
                break;
            }
        }
        break;
    case FitSize::All:
        from = columns.first();
        to = columns.last();
        break;
    case FitSize::ToEnd:
        from = active;
        to = columns.last();
        break;
    case FitSize::ToBeg:
        from = columns.first();
        to = active;
        break;
    default:
        return;
    }

    // Now align from to left edge of the screen (max.x), split width of
    // screen (max.w) among from->to, and readapt the rest
    if (from != nullptr && to != nullptr) {
        double total = 0.0;
        for (auto c = from; c != to->next(); c = c->next()) {
            total += c->data()->get_geom_w();
        }
        for (auto c = from; c != to->next(); c = c->next()) {
            Column *col = c->data();
            col->set_width_free();
            col->set_geom_w(col->get_geom_w() / total * max.w);
        }
        from->data()->set_geom_pos(max.x, max.y);

        adjust_columns(from);
    }
}

bool Row::is_overview() const
{
    return overview;
}

void Row::toggle_overview()
{
    overview = !overview;
    post_event("overview");
    static auto *const *overview_scale_content = (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:overview_scale_content")->getDataStaticPtr();
    if (overview) {
        // Turn off fullscreen mode if enabled
        auto window = get_active_window();
        preoverview_fsmode = window_fullscreen_state(window);
        if (preoverview_fsmode != eFullscreenMode::FSMODE_NONE) {
            toggle_window_fullscreen_internal(window, preoverview_fsmode);
        }
        // Find the bounding box
        Vector2D bmin(max.x + max.w, max.y + max.h);
        Vector2D bmax(max.x, max.y);
        for (auto c = columns.first(); c != nullptr; c = c->next()) {
            auto cx0 = c->data()->get_geom_x();
            auto cx1 = cx0 + c->data()->get_geom_w();
            Vector2D cheight = c->data()->get_height();
            if (cx0 < bmin.x)
                bmin.x = cx0;
            if (cx1 > bmax.x)
                bmax.x = cx1;
            if (cheight.x < bmin.y)
                bmin.y = cheight.x;
            if (cheight.y > bmax.y)
                bmax.y = cheight.y;
        }
        double w = bmax.x - bmin.x;
        double h = bmax.y - bmin.y;
        double scale = std::min(max.w / w, max.h / h);

        bool overview_scaled;
        if (**overview_scale_content && overviews->enable(workspace)) {
            overview_scaled = true;
        } else {
            overview_scaled = false;
        }
        if (overview_scaled) {
            Vector2D offset(0.5 * (max.w - w * scale), 0.5 * (max.h - h * scale));
            for (auto c = columns.first(); c != nullptr; c = c->next()) {
                Column *col = c->data();
                col->push_overview_geom();
                Vector2D cheight = col->get_height();
                col->set_geom_pos(full.x + (max.x - full.x + offset.x) / scale + (col->get_geom_x() - bmin.x), full.y + (max.y - full.y + offset.y) / scale + (cheight.x - bmin.y));
            }
            adjust_overview_columns();

            PHLMONITOR monitor = window->m_pWorkspace->m_pMonitor.lock();
            g_pHyprRenderer->damageMonitor(monitor);

            overviews->set_scale(workspace, scale);
            overviews->set_vecsize(workspace, monitor->vecSize);
            // Update cursor
            g_pCompositor->warpCursorTo(get_active_window()->middle());
        } else {
            Vector2D offset(0.5 * (max.w - w * scale), 0.5 * (max.h - h * scale));
            for (auto c = columns.first(); c != nullptr; c = c->next()) {
                Column *col = c->data();
                col->push_overview_geom();
                Vector2D cheight = col->get_height();
                col->set_geom_pos(offset.x + max.x + (col->get_geom_x() - bmin.x) * scale, offset.y + max.y + (cheight.x - bmin.y) * scale);
                col->set_geom_w(col->get_geom_w() * scale);
                Vector2D start(offset.x + max.x, offset.y + max.y);
                col->scale(bmin, start, scale, gap);
            }
            adjust_overview_columns();
        }
    } else {
        if (**overview_scale_content && overviews->is_initialized()) {
            PHLMONITOR monitor = get_active_window()->m_pWorkspace->m_pMonitor.lock();
            overviews->disable(workspace);
            g_pHyprRenderer->damageMonitor(monitor);
        }
        for (auto c = columns.first(); c != nullptr; c = c->next()) {
            Column *col = c->data();
            col->pop_overview_geom();
        }
        // Try to maintain the positions except if the active is not visible,
        // in that case, make it visible.
        Column *acolumn = active->data();
        if (acolumn->get_geom_x() < max.x) {
            acolumn->set_geom_pos(max.x, max.y);
        } else if (acolumn->get_geom_x() + acolumn->get_geom_w() > max.x + max.w) {
            acolumn->set_geom_pos(max.x + max.w - acolumn->get_geom_w(), max.y);
        }
        adjust_columns(active);
        // Turn fullscreen mode back on if enabled
        auto window = get_active_window();
        g_pCompositor->warpCursorTo(window->middle());
        if (preoverview_fsmode != eFullscreenMode::FSMODE_NONE) {
            toggle_window_fullscreen_internal(window, preoverview_fsmode);
        }
    }
}

void Row::update_windows(const Box &oldmax, bool force)
{
    if (!force)
        return;

    // Update active column position
    if (active && oldmax != max) {
        double posx = max.x + max.w * (active->data()->get_geom_x() - oldmax.x) / oldmax.w;
        double posy = max.y + max.h * (active->data()->get_geom_vy() - oldmax.y) / oldmax.h;
        active->data()->set_geom_pos(posx, posy);
    }
    // Redo all columns: widths according to "width" (unless Free)
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        Column *column = col->data();
        StandardSize width = column->get_width();
        double maxw = width == StandardSize::Free ? column->get_geom_w() : max.w;
        column->update_width(width, maxw);
        // Redo all windows for each column according to "height" (unless Free)
        column->update_heights();
    }
    recalculate_row_geometry();
}

void Row::recalculate_row_geometry()
{
    if (active == nullptr)
        return;

    if (active->data()->fullscreen()) {
        return;
    }
    if (overview) {
        adjust_overview_columns();
        return;
    }
    auto a_w = active->data()->get_geom_w();
    auto a_x = active->data()->get_geom_x();
    // Pinned will stay in place, with active having second priority to fit in
    // the screen on either side of pinned.
    if (pinned != nullptr) {
        // If pinned got kicked out of the screen (overview, for example),
        // bring it back in
        auto p_w = pinned->data()->get_geom_w();
        auto p_x = pinned->data()->get_geom_x();
        if (p_x < max.x) {
            pinned->data()->set_geom_pos(max.x, max.y);
        } else if (std::round(p_x + p_w) > max.x + max.w) {
            // pin overflows to the right, move to end of viewport
            pinned->data()->set_geom_pos(max.x + max.w - p_w, max.y);
        }
        if (a_x < max.x || std::round(a_x + a_w) > max.x + max.w) {
            // Active doesn't fit, move it next to pinned
            // Find space
            auto p_w = pinned->data()->get_geom_w();
            auto p_x = pinned->data()->get_geom_x();
            auto const lt = p_x - max.x;
            auto const rt = max.x + max.w - p_x - p_w;
            // From pinned to active, try to fit as many columns as possible
            if (pinned != active) {
                int p = 0, a = 0;
                int i = 0;
                for (auto col = columns.first(); col != nullptr; col = col->next(), ++i) {
                    if (col == pinned)
                        p = i;
                    if (col == active)
                        a = i;
                }
                if (p > a) {
                    // Pinned is after active
                    // The priority is to keep active before pinned if it fits.
                    // If it doesn't fit, see if it fits right after, otherwise
                    // move it to where there is more room, and if equal, leave
                    // it where it is.
                    auto w = a_w;
                    auto swap = active, col = active;
                    while (col != pinned) {
                        if (0 <= std::round(lt - w)) {
                            swap = col;
                            col = col->next();
                            w += col->data()->get_geom_w();
                        } else {
                            break;
                        }
                    }
                    if (0 <= std::round(lt - a_w)) {
                        // fits on the left
                        columns.move_after(swap, pinned);
                    } else {
                        if (0 <= std::round(rt - a_w) || rt > lt) {
                            // fits on the right
                            columns.move_before(active, pinned);
                        } else {
                            // doesn't fit, and there is the same or more
                            // room on the side where it is now, leave it
                            // there (right before pinned)
                            columns.move_after(active, pinned);
                        }
                    }
                } else {
                    // Pinned is before active
                    // The priority is to keep active after pinned if it fits.
                    // If it doesn't fit, see if it fits right before, otherwise
                    // move it to where there is more room, and if equal, leave
                    // it where it is.
                    auto w = a_w;
                    auto swap = active, col = active;
                    while (col != pinned) {
                        if (0 <= std::round(rt - w)) {
                            swap = col;
                            col = col->prev();
                            w += col->data()->get_geom_w();
                        } else {
                            break;
                        }
                    }
                    if (0 <= std::round(rt - a_w)) {
                        // fits on the right
                        columns.move_before(swap, pinned);
                    } else {
                        if (0 <= std::round(lt - a_w) || lt > rt) {
                            // fits on the left or there is more room there
                            columns.move_after(active, pinned);
                        } else {
                            // doesn't fit, and there is the same or more
                            // room on the side where it is now, leave it
                            // there (right after pinned)
                            columns.move_before(active, pinned);
                        }
                    }
                }
            }
        }
        // Now, we know pinned is in the right position (it doesn't move)
        adjust_columns(pinned);
        return;
    }

    if (a_x < max.x) {
        // active starts outside on the left
        // set it on the left edge
        active->data()->set_geom_pos(max.x, max.y);
    } else if (std::round(a_x + a_w) > max.x + max.w) {
        // active overflows to the right, move to end of viewport
        active->data()->set_geom_pos(max.x + max.w - a_w, max.y);
    } else {
        // active is inside the viewport
        if (reorder == Reorder::Auto) {
            // The active column should always be completely in the viewport.
            // If any of the windows next to it on its right or left are
            // in the viewport, keep the current position.
            bool keep_current = false;
            if (active->prev() != nullptr) {
                Column *prev = active->prev()->data();
                if (prev->get_geom_x() >= max.x && std::round(prev->get_geom_x() + prev->get_geom_w()) <= max.x + max.w) {
                    keep_current = true;
                }
            }
            if (!keep_current && active->next() != nullptr) {
                Column *next = active->next()->data();
                if (next->get_geom_x() >= max.x && std::round(next->get_geom_x() + next->get_geom_w()) <= max.x + max.w) {
                    keep_current = true;
                }
            }
            if (!keep_current) {
                // If not:
                // We try to fit the column next to it on the right if it fits
                // completely, otherwise the one on the left. If none of them fit,
                // we leave it as it is.
                if (active->next() != nullptr) {
                    if (std::round(a_w + active->next()->data()->get_geom_w()) <= max.w) {
                        // set next at the right edge of the viewport
                        active->data()->set_geom_pos(max.x + max.w - a_w - active->next()->data()->get_geom_w(), max.y);
                    } else if (active->prev() != nullptr) {
                        if (std::round(active->prev()->data()->get_geom_w() + a_w) <= max.w) {
                            // set previous at the left edge of the viewport
                            active->data()->set_geom_pos(max.x + active->prev()->data()->get_geom_w(), max.y);
                        } else {
                            // none of them fit, leave active as it is
                            active->data()->set_geom_pos(a_x, max.y);
                        }
                    } else {
                        // nothing on the left, move active to left edge of viewport
                        active->data()->set_geom_pos(max.x, max.y);
                    }
                } else if (active->prev() != nullptr) {
                    if (std::round(active->prev()->data()->get_geom_w() + a_w) <= max.w) {
                        // set previous at the left edge of the viewport
                        active->data()->set_geom_pos(max.x + active->prev()->data()->get_geom_w(), max.y);
                    } else {
                        // it doesn't fit and nothing on the right, move active to right edge of viewport
                        active->data()->set_geom_pos(max.x + max.w - a_w, max.y);
                    }
                } else {
                    // nothing on the right or left, the window is in a correct position
                    active->data()->set_geom_pos(a_x, max.y);
                }
            } else {
                // the window is in a correct position
                // if the window is first or last, and some windows don't fit,
                // ensure it is at the edge
                // Columns can be unsorted when calling this function, so get the full
                // width by adding all widths
                double w = 0.0;
                for (auto col = columns.first(); col != nullptr; col = col->next()) {
                    w += col->data()->get_geom_w();
                }
                if (std::round(w) >= max.w) {
                    if (active == columns.first()) {
                        active->data()->set_geom_pos(max.x, max.y);
                    } else if (active == columns.last()) {
                        active->data()->set_geom_pos(max.x + max.w - a_w, max.y);
                    } else {
                        active->data()->set_geom_pos(a_x, max.y);
                    }
                } else {
                    active->data()->set_geom_pos(a_x, max.y);
                }
            }
        } else { // lazy
            // Try to avoid moving the active column unless it is out of the screen.
            // the window is in a correct position
            active->data()->set_geom_pos(a_x, max.y);
        }
    }

    adjust_columns(active);
}

// Adjust all the columns in the row using 'column' as anchor
void Row::adjust_columns(ListNode<Column *> *column)
{
    // Adjust the positions of the columns to the left
    for (auto col = column->prev(), prev = column; col != nullptr; prev = col, col = col->prev()) {
        col->data()->set_geom_pos(prev->data()->get_geom_x() - col->data()->get_geom_w(), max.y);
    }
    // Adjust the positions of the columns to the right
    for (auto col = column->next(), prev = column; col != nullptr; prev = col, col = col->next()) {
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

// Adjust all the columns in the overview
void Row::adjust_overview_columns()
{
    // Apply column geometry
    for (auto col = columns.first(); col != nullptr; col = col->next()) {
        // First and last columns need a different gap
        auto gap0 = col == columns.first() ? 0.0 : gap;
        auto gap1 = col == columns.last() ? 0.0 : gap;
        col->data()->recalculate_col_geometry_overview(Vector2D(gap0, gap1), gap);
    }
}
