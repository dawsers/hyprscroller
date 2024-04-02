#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/VarList.hpp>
#include <hyprland/src/includes.hpp>
#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <optional>

#include "dispatchers.h"
#include "scroller.h"


extern HANDLE PHANDLE;
extern std::unique_ptr<ScrollerLayout> g_ScrollerLayout;


namespace {
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
        if (g_pCompositor->m_pLastMonitor->specialWorkspaceID) {
            workspace_id = g_pCompositor->m_pLastMonitor->specialWorkspaceID;
        } else {
            workspace_id = g_pCompositor->m_pLastMonitor->activeWorkspace;
        }
        if (workspace_id == WORKSPACE_INVALID)
            return -1;
        auto *workspace = g_pCompositor->getWorkspaceByID(workspace_id);
        if (workspace == nullptr)
            return -1;
        if (workspace->m_bHasFullscreenWindow)
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
    void dispatch_resetheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->reset_height(workspace);
    }
    void dispatch_toggleheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->toggle_height(workspace);
    }
    std::optional<FitWidth> parse_fit_width(std::string arg) {
        if (arg == "active")
            return FitWidth::Active;
        else if (arg == "visible")
            return FitWidth::Visible;
        else if (arg == "all")
            return FitWidth::All;
        else if (arg == "toend")
            return FitWidth::ToEnd;
        else if (arg == "tobeg" || arg == "tobeginning")
            return FitWidth::ToBeg;
        else
            return {};
    }
    void dispatch_fitwidth(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        auto args = CVarList(arg);
        if (auto fitwidth = parse_fit_width(args[0])) {
            g_ScrollerLayout->fit_width(workspace, *fitwidth);
        }
    }
    void dispatch_overview(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return;

        g_ScrollerLayout->toggle_overview(workspace);
    }
}

void dispatchers::addDispatchers() {
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:cyclesize", dispatch_cyclesize);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:movefocus", dispatch_movefocus);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:movewindow", dispatch_movewindow);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:alignwindow", dispatch_alignwindow);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:admitwindow", dispatch_admitwindow);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:expelwindow", dispatch_expelwindow);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:resetheight", dispatch_resetheight);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:toggleheight", dispatch_toggleheight);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:fitwidth", dispatch_fitwidth);
    HyprlandAPI::addDispatcher(PHANDLE, "scroller:overview", dispatch_overview);
}
