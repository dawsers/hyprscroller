#include "functions.h"
#include "scroller.h"
#include "dispatchers.h"

SDispatchResult this_moveFocusTo(std::string args)
{
    dispatchers::dispatch_movefocus(args);
    return {};
}


SDispatchResult this_moveActiveTo(std::string args)
{
    dispatchers::dispatch_movewindow(args);
    return {};
}


eFullscreenMode window_fullscreen_state(PHLWINDOW window)
{
    return window->m_sFullscreenState.internal;
}

void toggle_window_fullscreen_internal(PHLWINDOW window, eFullscreenMode mode)
{
    if (window_fullscreen_state(window) != eFullscreenMode::FSMODE_NONE) {
        g_pCompositor->setWindowFullscreenInternal(window, FSMODE_NONE);
    } else {
        g_pCompositor->setWindowFullscreenInternal(window, mode);
    }
}

WORKSPACEID get_workspace_id()
{
    WORKSPACEID workspace_id;
    if (g_pCompositor->m_pLastMonitor->activeSpecialWorkspaceID()) {
        workspace_id = g_pCompositor->m_pLastMonitor->activeSpecialWorkspaceID();
    } else {
        workspace_id = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
    }
    if (workspace_id == WORKSPACE_INVALID)
        return -1;
    if (g_pCompositor->getWorkspaceByID(workspace_id) == nullptr)
        return -1;

    return workspace_id;
}


void force_focus_to_window(PHLWINDOW window)
{
    g_pInputManager->unconstrainMouse();
    g_pCompositor->focusWindow(window);
    g_pCompositor->warpCursorTo(window->middle());

    g_pInputManager->m_pForcedFocus = window;
    g_pInputManager->simulateMouseMovement();
    g_pInputManager->m_pForcedFocus.reset();
}

void switch_to_window(PHLWINDOW from, PHLWINDOW to)
{
    if (to == nullptr)
        return;

    auto fwid = from != nullptr? from->workspaceID() : WORKSPACE_INVALID;
    auto twid = to->workspaceID();
    bool change_workspace = fwid != twid;
    if (from != to) {
        const PHLWORKSPACE workspace = to->m_pWorkspace;
        eFullscreenMode mode = workspace->m_efFullscreenMode;
        if (mode != eFullscreenMode::FSMODE_NONE) {
            if (change_workspace) {
                auto fwindow = workspace->getLastFocusedWindow(); 
                toggle_window_fullscreen_internal(fwindow, eFullscreenMode::FSMODE_NONE);
            } else {
                toggle_window_fullscreen_internal(from, eFullscreenMode::FSMODE_NONE);
            }
        }
        if (change_workspace) {
            // This is to override overview trying to stay in an overview workspace
            g_pCompositor->m_pLastMonitor = to->m_pMonitor;
        }
        force_focus_to_window(to);
        if (mode != eFullscreenMode::FSMODE_NONE) {
            toggle_window_fullscreen_internal(to, mode);
        }
    } else {
        // from and to are the same, it can happen when we want to recover
        // focus after changing to another monitor where focus was lost
        // due to a window exiting in the background
        force_focus_to_window(to);
    }
    return;
}

