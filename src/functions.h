#ifndef SCROLLER_FUNCTIONS_H
#define SCROLLER_FUNCTIONS_H

#include <hyprland/src/helpers/Timer.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>

SDispatchResult this_moveFocusTo(std::string args);
SDispatchResult this_moveActiveTo(std::string args);
eFullscreenMode window_fullscreen_state(PHLWINDOW window);
void toggle_window_fullscreen_internal(PHLWINDOW window, eFullscreenMode mode);
WORKSPACEID get_workspace_id();
void update_relative_cursor_coords(PHLWINDOW window);
void force_focus_to_window(PHLWINDOW window);
void switch_to_window(PHLWINDOW from, PHLWINDOW to);

#endif // SCROLLER_FUNCTIONS_H
