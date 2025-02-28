#ifndef SCROLLER_SCROLLER_H
#define SCROLLER_SCROLLER_H

#include <hyprland/src/layout/IHyprLayout.hpp>

#include "list.h"
#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/devices/IPointer.hpp>

#include "enums.h"

class Row;

class ScrollerLayout : public IHyprLayout {
public:
    virtual void onEnable();
    virtual void onDisable();

    virtual void onWindowCreatedTiling(PHLWINDOW,
                                       eDirection = DIRECTION_DEFAULT);
    virtual bool isWindowTiled(PHLWINDOW);
    virtual void onWindowRemovedTiling(PHLWINDOW);
    virtual void onWindowRemovedFloating(PHLWINDOW);
    virtual void recalculateMonitor(const MONITORID &monitor_id);
    virtual void recalculateWindow(PHLWINDOW);
    virtual void resizeActiveWindow(const Vector2D &delta, eRectCorner corner,
                                    PHLWINDOW pWindow = nullptr);
    virtual void fullscreenRequestForWindow(PHLWINDOW pWindow,
            const eFullscreenMode CURRENT_EFFECTIVE_MODE,
            const eFullscreenMode EFFECTIVE_MODE);
    virtual std::any layoutMessage(SLayoutMessageHeader header,
                                   std::string content);
    virtual SWindowRenderLayoutHints requestRenderHints(PHLWINDOW);
    virtual void switchWindows(PHLWINDOW, PHLWINDOW);
    virtual void moveWindowTo(PHLWINDOW, const std::string &direction, bool silent = false);
    virtual void alterSplitRatio(PHLWINDOW, float, bool);
    virtual std::string getLayoutName();
    virtual PHLWINDOW getNextWindowCandidate(PHLWINDOW);
    virtual void onWindowFocusChange(PHLWINDOW);
    virtual void replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
    virtual Vector2D predictSizeForNewWindowTiled();

    // New Dispatchers
    void cycle_window_size(WORKSPACEID workspace, int step);
    void cycle_window_width(WORKSPACEID workspace, int step);
    void cycle_window_height(WORKSPACEID workspace, int step);
    void set_window_size(WORKSPACEID workspace, const std::string &arg);
    void set_window_width(WORKSPACEID workspace, const std::string &arg);
    void set_window_height(WORKSPACEID workspace, const std::string &arg);
    void move_focus(WORKSPACEID workspace, Direction);
    void move_window(WORKSPACEID workspace, Direction, bool);
    void align_window(WORKSPACEID workspace, Direction);
    void admit_window(WORKSPACEID workspace, AdmitExpelDirection direction);
    void expel_window(WORKSPACEID workspace, AdmitExpelDirection direction);
    void set_mode(WORKSPACEID workspace, Mode);
    void set_mode_modifier(WORKSPACEID workspace, const ModeModifier &);
    void fit_size(WORKSPACEID workspace, FitSize);
    void fit_width(WORKSPACEID workspace, FitSize);
    void fit_height(WORKSPACEID workspace, FitSize);
    void toggle_overview(WORKSPACEID workspace);

    void marks_add(const std::string &name);
    void marks_delete(const std::string &name);
    void marks_visit(const std::string &name);
    void marks_reset();

    void pin(WORKSPACEID workspace);

    void selection_toggle(WORKSPACEID workspace);
    void selection_set(PHLWINDOWREF window);
    void selection_reset();
    void selection_workspace(WORKSPACEID workspace);
    void selection_move(WORKSPACEID workspace, Direction direction = Direction::End);

    void trail_new();
    void trail_next();
    void trail_prev();
    void trail_delete();
    void trail_clear();
    void trail_toselection();
    void trailmark_toggle();
    void trailmark_next();
    void trailmark_prev();

    void jump();

    void post_event(WORKSPACEID workspace, const std::string &event);

    void swipe_begin(IPointer::SSwipeBeginEvent);
    void swipe_update(SCallbackInfo& info, IPointer::SSwipeUpdateEvent);
    void swipe_end(SCallbackInfo& info, IPointer::SSwipeEndEvent);

    void mouse_move(SCallbackInfo& info, const Vector2D &mousePos);

    bool is_enabled() const { return enabled; }

private:
    Row *getRowForWorkspace(WORKSPACEID workspace);
    Row *getRowForWindow(PHLWINDOW window);
    PHLWINDOW getActiveWindow(WORKSPACEID workspace);

    List<Row *> rows;

    bool enabled;
    Vector2D gesture_delta;
    bool swipe_active;
    Direction swipe_direction;
    bool jumping = false;
};

#endif  // SCROLLER_SCROLLER_H
