#include <hyprland/src/Compositor.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprland/src/includes.hpp>
#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <optional>

#include "dispatchers.h"
#include "scroller.h"


extern HANDLE PHANDLE;
extern std::unique_ptr<ScrollerLayout> g_ScrollerLayout;


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
        else
            return {};
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

    void dispatch_cyclesize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        int step = 0;
        if (arg == "+1" || arg == "1" || arg == "next") {
            step = 1;
        } else if (arg == "-1" || arg == "prev" || arg == "previous") {
            step = -1;
        } else {
            return;
        }
        g_ScrollerLayout->cycle_window_size(workspace, step);
    }

    void dispatch_movefocus(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            g_ScrollerLayout->move_focus(workspace, *direction);
        }
    }

    void dispatch_movewindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            g_ScrollerLayout->move_window(workspace, *direction);
        }
    }

    void dispatch_alignwindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            g_ScrollerLayout->align_window(workspace, *direction);
        }
    }

    void dispatch_admitwindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->admit_window_left(workspace);
    }

    void dispatch_expelwindow(std::string arg) {
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
    void dispatch_toggleoverview(std::string arg) {
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
        g_ScrollerLayout->marks_delete(arg);
    }
    void dispatch_marksvisit(std::string arg) {
        g_ScrollerLayout->marks_visit(arg);
    }
    void dispatch_marksreset(std::string arg) {
        g_ScrollerLayout->marks_reset();
    }
    void dispatch_pin(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->pin(workspace);
    }
    void dispatch_unpin(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->unpin(workspace);
    }
    void dispatch_selectiontoggle(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->selection_toggle(workspace);
    }
    void dispatch_selectionreset(std::string arg) {
        g_ScrollerLayout->selection_reset();
    }
    void dispatch_selectionmove(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            g_ScrollerLayout->selection_move(workspace, *direction);
        }
    }
    void addDispatchers() {
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:cyclesize", dispatch_cyclesize);
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
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:unpin", dispatch_unpin);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectiontoggle", dispatch_selectiontoggle);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectionreset", dispatch_selectionreset);
        HyprlandAPI::addDispatcher(PHANDLE, "scroller:selectionmove", dispatch_selectionmove);
    }
}
