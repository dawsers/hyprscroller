#include <hyprland/src/layout/IHyprLayout.hpp>

#include "list.h"

enum class Direction { Left, Right, Up, Down, Begin, End, Center };
enum class FitWidth { Active, Visible, All, ToEnd, ToBeg };

class Row;

class ScrollerLayout : public IHyprLayout {
public:
    virtual void onEnable();
    virtual void onDisable();

    virtual void onWindowCreatedTiling(CWindow *,
                                       eDirection = DIRECTION_DEFAULT);
    virtual bool isWindowTiled(CWindow *);
    virtual void onWindowRemovedTiling(CWindow *);
    virtual void recalculateMonitor(const int &monitor_id);
    virtual void recalculateWindow(CWindow *);
    virtual void resizeActiveWindow(const Vector2D &delta, eRectCorner corner,
                                    CWindow *pWindow = nullptr);
    virtual void fullscreenRequestForWindow(CWindow *, eFullscreenMode,
                                            bool enable_fullscreen);
    virtual std::any layoutMessage(SLayoutMessageHeader header,
                                   std::string content);
    virtual SWindowRenderLayoutHints requestRenderHints(CWindow *);
    virtual void switchWindows(CWindow *, CWindow *);
    virtual void moveWindowTo(CWindow *, const std::string &direction);
    virtual void alterSplitRatio(CWindow *, float, bool);
    virtual std::string getLayoutName();
    virtual CWindow *getNextWindowCandidate(CWindow *);
    virtual void onWindowFocusChange(CWindow *);
    virtual void replaceWindowDataWith(CWindow *from, CWindow *to);
    virtual Vector2D predictSizeForNewWindowTiled();

    // New Dispatchers
    void cycle_window_size(int workspace, int step);
    void move_focus(int workspace, Direction);
    void move_window(int workspace, Direction);
    void align_window(int workspace, Direction);
    void admit_window_left(int workspace);
    void expel_window_right(int workspace);
    void reset_height(int workspace);
    void toggle_height(int workspace);
    void fit_width(int workspace, FitWidth);

private:
    Row *getRowForWorkspace(int workspace);
    Row *getRowForWindow(CWindow *window);

    List<Row *> rows;
};
