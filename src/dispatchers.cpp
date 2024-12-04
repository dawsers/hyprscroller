#include <hyprland/src/Compositor.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprland/src/includes.hpp>
#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <optional>

#include "enums.h"
#include "dispatchers.h"
#include "scroller.h"


extern HANDLE PHANDLE;
extern std::unique_ptr<ScrollerLayout> g_ScrollerLayout;
extern std::function<SDispatchResult(std::string)> orig_moveActiveTo;


namespace dispatchers {
    std::optional<Direction> parse_move_arg(std::string arg) {
        if (arg == "l" || arg == "left")
            return Direction::Left;
        else if (arg == "r" || arg == "right")
            return Direction::Right;
        else if (arg == "u" || arg == "up")
            return Direction::Up;
        else if (arg == "d" || arg == "dn" || arg == "down")
            return Direction::Down;
        else if (arg == "b" || arg == "begin" || arg == "beginning")
            return Direction::Begin;
        else if (arg == "e" || arg == "end")
            return Direction::End;
        else if (arg == "c" || arg == "center" || arg == "centre")
            return Direction::Center;
        else if (arg == "m" || arg == "middle")
            return Direction::Middle;
        else
            return Direction::Invalid;
    }

    int workspace_for_action() {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return -1;

        int workspace_id;
        if (g_pCompositor->m_pLastMonitor->activeSpecialWorkspaceID()) {
            workspace_id = g_pCompositor->m_pLastMonitor->activeSpecialWorkspaceID();
        } else {
            workspace_id = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
        }
        if (workspace_id == WORKSPACE_INVALID)
            return -1;
        auto workspace = g_pCompositor->getWorkspaceByID(workspace_id);
        if (workspace == nullptr)
            return -1;

        return workspace_id;
    }

    int parse_step_arg(const std::string &arg) {
        int step = 0;
        if (arg == "+1" || arg == "1" || arg == "next") {
            step = 1;
        } else if (arg == "-1" || arg == "prev" || arg == "previous") {
            step = -1;
        }
        return step;
    }

    void dispatch_cyclesize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_size(workspace, step);
    }

    void dispatch_cyclewidth(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_width(workspace, step);
    }

    void dispatch_cycleheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_height(workspace, step);
    }

    void dispatch_movefocus(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->move_focus(workspace, *direction);
        }
    }

    void dispatch_movewindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->move_window(workspace, *direction);
            else
               orig_moveActiveTo(arg);
        }
    }

    void dispatch_alignwindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->align_window(workspace, *direction);
        }
    }

    void dispatch_admitwindow(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->admit_window_left(workspace);
    }

    void dispatch_expelwindow(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->expel_window_right(workspace);
    }
    void dispatch_setmode(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        Mode mode = Mode::Row;
        if (arg == "r" || arg == "row") {
            mode = Mode::Row;
        } else if (arg == "c" || arg == "col" || arg == "column") {
            mode = Mode::Column;
        }
        g_ScrollerLayout->set_mode(workspace, mode);
    }
    std::optional<FitSize> parse_fit_size(std::string arg) {
        if (arg == "active")
            return FitSize::Active;
        else if (arg == "visible")
            return FitSize::Visible;
        else if (arg == "all")
            return FitSize::All;
        else if (arg == "toend")
            return FitSize::ToEnd;
        else if (arg == "tobeg" || arg == "tobeginning")
            return FitSize::ToBeg;
        else
            return {};
    }
    void dispatch_fitsize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto fitsize = parse_fit_size(args[0])) {
            g_ScrollerLayout->fit_size(workspace, *fitsize);
        }
    }
    void dispatch_toggleoverview(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->toggle_overview(workspace);
    }
    void dispatch_marksadd(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->marks_add(arg);
    }
    void dispatch_marksdelete(std::string arg) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->marks_delete(arg);
    }
    void dispatch_marksvisit(std::string arg) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->marks_visit(arg);
    }
    void dispatch_marksreset(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->marks_reset();
    }
    void dispatch_pin(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->pin(workspace);
    }
    void dispatch_selectiontoggle(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->selection_toggle(workspace);
    }
    void dispatch_selectionreset(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->selection_reset();
    }
    void dispatch_selectionmove(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->selection_move(workspace, *direction);
        }
    }
    void dispatch_trailnew(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_new();
    }
    void dispatch_trailnext(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_next();
    }
    void dispatch_trailprev(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_prev();
    }
    void dispatch_traildelete(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_delete();
    }
    void dispatch_trailclear(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_clear();
    }
    void dispatch_trailtoselection(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trail_toselection();
    }
    void dispatch_trailmarktoggle(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trailmark_toggle();
    }
    void dispatch_trailmarknext(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trailmark_next();
    }
    void dispatch_trailmarkprev(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->trailmark_prev();
    }
    void dispatch_jump(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return;

        g_ScrollerLayout->jump();
    }
    void addDispatchers() {
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:cyclesize", dispatch_cyclesize);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:cyclewidth", dispatch_cyclewidth);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:cycleheight", dispatch_cycleheight);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:movefocus", dispatch_movefocus);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:movewindow", dispatch_movewindow);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:alignwindow", dispatch_alignwindow);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:admitwindow", dispatch_admitwindow);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:expelwindow", dispatch_expelwindow);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:setmode", dispatch_setmode);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:fitsize", dispatch_fitsize);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:toggleoverview", dispatch_toggleoverview);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:marksadd", dispatch_marksadd);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:marksdelete", dispatch_marksdelete);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:marksvisit", dispatch_marksvisit);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:marksreset", dispatch_marksreset);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:pin", dispatch_pin);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectiontoggle", dispatch_selectiontoggle);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectionreset", dispatch_selectionreset);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectionmove", dispatch_selectionmove);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailnew", dispatch_trailnew);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailnext", dispatch_trailnext);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailprevious", dispatch_trailprev);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:traildelete", dispatch_traildelete);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailclear", dispatch_trailclear);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailtoselection", dispatch_trailtoselection);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailmarktoggle", dispatch_trailmarktoggle);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailmarknext", dispatch_trailmarknext);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:trailmarkprevious", dispatch_trailmarkprev);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:jump", dispatch_jump);
    }
}
