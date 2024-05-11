#include <hyprland/src/layout/IHyprLayout.hpp>

#include "list.h"

enum class Direction { Left, Right, Up, Down, Begin, End, Center };
enum class FitSize { Active, Visible, All, ToEnd, ToBeg };
enum class Mode { Row, Column };

class Row;

class ScrollerLayout : public IHyprLayout {
public:
    virtual void onEnable();
    virtual void onDisable();

    virtual void onWindowCreatedTiling(PHLWINDOW,
                                       eDirection = DIRECTION_DEFAULT);
    virtual bool isWindowTiled(PHLWINDOW);
    virtual void onWindowRemovedTiling(PHLWINDOW);
    virtual void recalculateMonitor(const int &monitor_id);
    virtual void recalculateWindow(PHLWINDOW);
    virtual void resizeActiveWindow(const Vector2D &delta, eRectCorner corner,
                                    PHLWINDOW pWindow = nullptr);
    virtual void fullscreenRequestForWindow(PHLWINDOW, eFullscreenMode,
                                            bool enable_fullscreen);
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
    void cycle_window_size(int workspace, int step);
    void move_focus(int workspace, Direction);
    void move_window(int workspace, Direction);
    void align_window(int workspace, Direction);
    void admit_window_left(int workspace);
    void expel_window_right(int workspace);
    void reset_height(int workspace);
    void set_mode(int workspace, Mode);
    void fit_size(int workspace, FitSize);
    void toggle_overview(int workspace);

    void marks_add(const std::string &name);
    void marks_delete(const std::string &name);
    void marks_visit(const std::string &name);
    void marks_reset();

private:
    Row *getRowForWorkspace(int workspace);
    Row *getRowForWindow(PHLWINDOW window);

    List<Row *> rows;
};
