#include <hyprland/src/Compositor.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprland/src/includes.hpp>
#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
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

    SDispatchResult dispatch_cyclesize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:cyclesize: invalid workspace" };

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_size(workspace, step);

        return {};
    }

    SDispatchResult dispatch_cyclewidth(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:cyclewidth: invalid workspace" };

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_width(workspace, step);

        return {};
    }

    SDispatchResult dispatch_cycleheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:cycleheight: invalid workspace" };

        int step = parse_step_arg(arg);
        if (step != 0)
            g_ScrollerLayout->cycle_window_height(workspace, step);

        return {};
    }

    SDispatchResult dispatch_setsize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:setsize: invalid workspace" };

        g_ScrollerLayout->set_window_size(workspace, arg);

        return {};
    }

    SDispatchResult dispatch_setwidth(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:setwidth: invalid workspace" };

        g_ScrollerLayout->set_window_width(workspace, arg);

        return {};
    }

    SDispatchResult dispatch_setheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:setheight: invalid workspace" };

        g_ScrollerLayout->set_window_height(workspace, arg);

        return {};
    }

    SDispatchResult dispatch_movefocus(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:movefocus: invalid workspace" };

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->move_focus(workspace, *direction);
        }

        return {};
    }

    SDispatchResult dispatch_movewindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:movewindow: invalid workspace" };

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->move_window(workspace, *direction);
            else
               orig_moveActiveTo(arg);
        }

        return {};
    }

    SDispatchResult dispatch_alignwindow(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:alignwindow: invalid workspace" };

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->align_window(workspace, *direction);
        }

        return {};
    }

    SDispatchResult dispatch_admitwindow(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:admitwindow: invalid workspace" };

        g_ScrollerLayout->admit_window_left(workspace);

        return {};
    }

    SDispatchResult dispatch_expelwindow(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:expelwindow: invalid workspace" };

        g_ScrollerLayout->expel_window_right(workspace);

        return {};
    }
    SDispatchResult dispatch_setmode(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:setmode: invalid workspace" };

        Mode mode = Mode::Row;
        if (arg == "r" || arg == "row") {
            mode = Mode::Row;
        } else if (arg == "c" || arg == "col" || arg == "column") {
            mode = Mode::Column;
        }
        g_ScrollerLayout->set_mode(workspace, mode);

        return {};
    }
    SDispatchResult dispatch_setmodemodifier(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:setmodemodifier: invalid workspace" };

        ModeModifier modifier(arg);
        g_ScrollerLayout->set_mode_modifier(workspace, modifier);

        return {};
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
    SDispatchResult dispatch_fitsize(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:fitsize: invalid workspace" };

        auto args = CVarList(arg);
        if (auto fitsize = parse_fit_size(args[0])) {
            g_ScrollerLayout->fit_size(workspace, *fitsize);
        }

        return {};
    }
    SDispatchResult dispatch_fitwidth(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:fitwidth: invalid workspace" };

        auto args = CVarList(arg);
        if (auto fitsize = parse_fit_size(args[0])) {
            g_ScrollerLayout->fit_width(workspace, *fitsize);
        }

        return {};
    }
    SDispatchResult dispatch_fitheight(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:fitheight: invalid workspace" };

        auto args = CVarList(arg);
        if (auto fitsize = parse_fit_size(args[0])) {
            g_ScrollerLayout->fit_height(workspace, *fitsize);
        }

        return {};
    }
    SDispatchResult dispatch_toggleoverview(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:toggleoverview: invalid workspace" };

        g_ScrollerLayout->toggle_overview(workspace);

        return {};
    }
    SDispatchResult dispatch_marksadd(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:marksadd: invalid workspace" };

        g_ScrollerLayout->marks_add(arg);

        return {};
    }
    SDispatchResult dispatch_marksdelete(std::string arg) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:marksdelete: called while not running hyprscroller" };

        g_ScrollerLayout->marks_delete(arg);

        return {};
    }
    SDispatchResult dispatch_marksvisit(std::string arg) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:marksvisit: called while not running hyprscroller" };

        g_ScrollerLayout->marks_visit(arg);

        return {};
    }
    SDispatchResult dispatch_marksreset(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:marksreset: called while not running hyprscroller" };

        g_ScrollerLayout->marks_reset();

        return {};
    }
    SDispatchResult dispatch_pin(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:pin: invalid workspace" };

        g_ScrollerLayout->pin(workspace);

        return {};
    }
    SDispatchResult dispatch_selectiontoggle(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:selectiontoggle: invalid workspace" };

        g_ScrollerLayout->selection_toggle(workspace);

        return {};
    }
    SDispatchResult dispatch_selectionreset(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:selectionreset: called while not running hyprscroller" };

        g_ScrollerLayout->selection_reset();

        return {};
    }
    SDispatchResult dispatch_selectionworkspace(std::string) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:selectionworkspace: invalid workspace" };

        g_ScrollerLayout->selection_workspace(workspace);

        return {};
    }
    SDispatchResult dispatch_selectionmove(std::string arg) {
        auto workspace = workspace_for_action();
        if (workspace == -1)
            return { .success = false, .error = "scroller:selectionmove: invalid workspace" };

        auto args = CVarList(arg);
        if (auto direction = parse_move_arg(args[0])) {
            if (direction != Direction::Invalid)
                g_ScrollerLayout->selection_move(workspace, *direction);
        }

        return {};
    }
    SDispatchResult dispatch_trailnew(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailnew: called while not running hyprscroller" };

        g_ScrollerLayout->trail_new();

        return {};
    }
    SDispatchResult dispatch_trailnext(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailnext: called while not running hyprscroller" };

        g_ScrollerLayout->trail_next();

        return {};
    }
    SDispatchResult dispatch_trailprev(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailprevious: called while not running hyprscroller" };

        g_ScrollerLayout->trail_prev();

        return {};
    }
    SDispatchResult dispatch_traildelete(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:traildelete: called while not running hyprscroller" };

        g_ScrollerLayout->trail_delete();

        return {};
    }
    SDispatchResult dispatch_trailclear(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailclear: called while not running hyprscroller" };

        g_ScrollerLayout->trail_clear();

        return {};
    }
    SDispatchResult dispatch_trailtoselection(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailtoselection: called while not running hyprscroller" };

        g_ScrollerLayout->trail_toselection();

        return {};
    }
    SDispatchResult dispatch_trailmarktoggle(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailmarktoggle: called while not running hyprscroller" };

        g_ScrollerLayout->trailmark_toggle();

        return {};
    }
    SDispatchResult dispatch_trailmarknext(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailmarknext: called while not running hyprscroller" };

        g_ScrollerLayout->trailmark_next();

        return {};
    }
    SDispatchResult dispatch_trailmarkprev(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:trailmarkprevious: called while not running hyprscroller" };

        g_ScrollerLayout->trailmark_prev();

        return {};
    }
    SDispatchResult dispatch_jump(std::string) {
        if (g_pLayoutManager->getCurrentLayout() != g_ScrollerLayout.get())
            return { .success = false, .error = "scroller:jump: called while not running hyprscroller" };

        g_ScrollerLayout->jump();

        return {};
    }
    void addDispatchers() {
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:cyclesize", dispatch_cyclesize);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:cyclewidth", dispatch_cyclewidth);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:cycleheight", dispatch_cycleheight);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:setsize", dispatch_setsize);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:setwidth", dispatch_setwidth);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:setheight", dispatch_setheight);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:movefocus", dispatch_movefocus);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:movewindow", dispatch_movewindow);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:alignwindow", dispatch_alignwindow);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:admitwindow", dispatch_admitwindow);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:expelwindow", dispatch_expelwindow);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:setmode", dispatch_setmode);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:setmodemodifier", dispatch_setmodemodifier);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:fitsize", dispatch_fitsize);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:fitwidth", dispatch_fitwidth);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:fitheight", dispatch_fitheight);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:toggleoverview", dispatch_toggleoverview);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:marksadd", dispatch_marksadd);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:marksdelete", dispatch_marksdelete);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:marksvisit", dispatch_marksvisit);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:marksreset", dispatch_marksreset);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:pin", dispatch_pin);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:selectiontoggle", dispatch_selectiontoggle);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:selectionreset", dispatch_selectionreset);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:selectionworkspace", dispatch_selectionworkspace);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:selectionmove", dispatch_selectionmove);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailnew", dispatch_trailnew);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailnext", dispatch_trailnext);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailprevious", dispatch_trailprev);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:traildelete", dispatch_traildelete);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailclear", dispatch_trailclear);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailtoselection", dispatch_trailtoselection);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailmarktoggle", dispatch_trailmarktoggle);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailmarknext", dispatch_trailmarknext);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:trailmarkprevious", dispatch_trailmarkprev);
        HyprlandAPI::addDispatcherV2(PHANDLE, "scroller:jump", dispatch_jump);
    }
}
