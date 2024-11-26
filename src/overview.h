#ifndef SCROLLER_OVERVIEW_H
#define SCROLLER_OVERVIEW_H

#include <hyprland/src/SharedDefs.hpp>
#include <unordered_map>

class Overview {
public:
    Overview();
    ~Overview();
    bool is_initialized() const { return initialized; }
    bool enable(WORKSPACEID workspace);
    void disable(WORKSPACEID workspace);
    bool overview_enabled(WORKSPACEID workspace) const;
    void set_vecsize(WORKSPACEID workspace, const Vector2D &size);
    void set_scale(WORKSPACEID workspace, float scale);
    Vector2D get_vecsize(WORKSPACEID workspace) const;
    float get_scale(WORKSPACEID workspace) const;

private:
    bool overview_enabled() const;
    bool enable_hooks();
    void disable_hooks();

    typedef struct {
        bool overview;
        Vector2D vecsize;
        float scale;
    } OverviewData;

    bool initialized;
    std::unordered_map<WORKSPACEID, OverviewData> workspaces;
};

#endif // SCROLLER_OVERVIEW_H
