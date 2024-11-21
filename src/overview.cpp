#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include "overview.h"

extern HANDLE PHANDLE;

inline CFunctionHook* g_pVisibleOnMonitorHook = nullptr;
inline CFunctionHook* g_pRenderLayerHook = nullptr;
inline CFunctionHook* g_pLogicalBoxHook = nullptr;
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;
inline CFunctionHook* g_pGetMonitorFromVectorHook = nullptr;
inline CFunctionHook* g_pClosestValidHook = nullptr;
inline CFunctionHook* g_pRenderMonitorHook = nullptr;
inline CFunctionHook* g_pGetCursorPosForMonitorHook = nullptr;

Overview *overviews = nullptr;

typedef bool (*origVisibleOnMonitor)(void *thisptr, PHLMONITOR monitor);
typedef void (*origRenderLayer)(void *thisptr, PHLLS pLayer, PHLMONITOR pMonitor, timespec* time, bool popups);
typedef CBox (*origLogicalBox)(void *thisptr);
typedef void (*origRenderSoftwareCursorsFor)(void *thisptr, PHLMONITOR pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos);
typedef Vector2D (*origClosestValid)(void *thisptr, const Vector2D &pos);
typedef PHLMONITOR (*origGetMonitorFromVector)(void *thisptr, const Vector2D& point);
typedef void (*origRenderMonitor)(void *thisptr, PHLMONITOR pMonitor);
typedef Vector2D (*origGetCursorPosForMonitor)(void *thisptr, PHLMONITOR pMonitor);

// Needed to show windows that are outside of the viewport
static bool hookVisibleOnMonitor(void *thisptr, PHLMONITOR monitor) {
    CWindow *window = static_cast<CWindow *>(thisptr);
    if (overviews->overview_enabled(window->workspaceID())) {
        return true;
    }
    return ((origVisibleOnMonitor)(g_pVisibleOnMonitorHook->m_pOriginal))(thisptr, monitor);
}

// Needed to undo the monitor scale to render layers at the original scale
static void hookRenderLayer(void *thisptr, PHLLS layer, PHLMONITOR monitor, timespec* time, bool popups) {
    WORKSPACEID workspace = monitor->activeSpecialWorkspaceID();
    if (!workspace)
        workspace = monitor->activeWorkspaceID();
    bool overview_enabled = overviews->overview_enabled(workspace);
    float scale = monitor->scale;
    if (overview_enabled) {
        monitor->scale /= overviews->get_scale(workspace);
        g_pHyprRenderer->damageMonitor(monitor);
    }
    ((origRenderLayer)(g_pRenderLayerHook->m_pOriginal))(thisptr, layer, monitor, time, popups);
    if (overview_enabled) {
        monitor->scale = scale;
        g_pHyprRenderer->damageMonitor(monitor);
    }
}

// Needed to scale the range of the cursor in overview mode to cover the whole area.
static CBox hookLogicalBox(void *thisptr) {
    CMonitor *monitor = static_cast<CMonitor *>(thisptr);
    if (g_pCompositor->m_pLastMonitor.get() != monitor)
        return CBox();

    WORKSPACEID workspace = monitor->activeSpecialWorkspaceID();
    if (!workspace)
        workspace = monitor->activeWorkspaceID();
    bool overview_enabled = overviews->overview_enabled(workspace);
    Vector2D monitor_size = monitor->vecSize;
    if (overview_enabled) {
        monitor->vecSize = overviews->get_vecsize(workspace) / overviews->get_scale(workspace);
    }
    CBox box = ((origLogicalBox)(g_pLogicalBoxHook->m_pOriginal))(thisptr);
    if (overview_enabled) {
        monitor->vecSize = monitor_size;
    }
    return box;
}

// Needed to render the HW cursor at the right position
static Vector2D hookGetCursorPosForMonitor(void *thisptr, PHLMONITOR monitor) {
    WORKSPACEID workspace = monitor->activeSpecialWorkspaceID();
    if (!workspace)
        workspace = monitor->activeWorkspaceID();
    bool overview_enabled = overviews->overview_enabled(workspace);
    double monitor_scale = monitor->scale;
    if (overview_enabled) {
        monitor->scale *= overviews->get_scale(workspace);
    }
    Vector2D pos = ((origGetCursorPosForMonitor)(g_pGetCursorPosForMonitorHook->m_pOriginal))(thisptr, monitor);
    if (overview_enabled) {
        monitor->scale = monitor_scale;
    }
    return pos;
}

// Needed to render the software cursor only on the correct monitors.
static void hookRenderSoftwareCursorsFor(void *thisptr, PHLMONITOR monitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    // Should render the cursor for all the extent of the workspace, and only on
    // overview workspaces when there is one active, and it is in the current monitor.
    PHLMONITOR last = g_pCompositor->m_pLastMonitor.lock();
    PHLMONITOR mon;
    bool overview_enabled = false;
    for (auto const& m : g_pCompositor->m_vMonitors) {
        WORKSPACEID workspace = m->activeSpecialWorkspaceID();
        if (!workspace)
            workspace = m->activeWorkspaceID();
        if (overviews->overview_enabled(workspace)) {
            overview_enabled = true;
            break;
        }
    }

    if (monitor == last) {
        // Render cursor
        WORKSPACEID workspace = monitor->activeSpecialWorkspaceID();
        if (!workspace)
            workspace = monitor->activeWorkspaceID();
        if (overviews->overview_enabled(workspace)) {
            Vector2D monitor_size = monitor->vecSize;
            monitor->vecSize = overviews->get_vecsize(workspace) / overviews->get_scale(workspace);
            ((origRenderSoftwareCursorsFor)(g_pRenderSoftwareCursorsForHook->m_pOriginal))(thisptr, monitor, now, damage, overridePos);
            monitor->vecSize = monitor_size;
        }
    } else if (overview_enabled) {
        return;
    }
    ((origRenderSoftwareCursorsFor)(g_pRenderSoftwareCursorsForHook->m_pOriginal))(thisptr, monitor, now, damage, overridePos);
}

// Needed to fake an overview monitor's desktop contains all its windows
// instead of some of them being in the other monitor.
static Vector2D hookClosestValid(void *thisptr, const Vector2D& pos) {
    PHLMONITOR last = g_pCompositor->m_pLastMonitor.lock();
    WORKSPACEID workspace = last->activeSpecialWorkspaceID();
    if (!workspace)
        workspace = last->activeWorkspaceID();
    bool overview_enabled = overviews->overview_enabled(workspace);
    if (overview_enabled)
       return pos;
    return ((origClosestValid)(g_pClosestValidHook->m_pOriginal))(thisptr, pos);
}

// Needed to select the correct monitor for a cursor when two can contain it.
static PHLMONITOR hookGetMonitorFromVector(void *thisptr, const Vector2D& point) {
    CCompositor *compositor = static_cast<CCompositor *>(thisptr);
    // First, see if the current monitor contains the point
    PHLMONITOR last = compositor->m_pLastMonitor.lock();
    PHLMONITOR mon;
    for (auto const& m : compositor->m_vMonitors) {
        WORKSPACEID workspace = m->activeSpecialWorkspaceID();
        if (!workspace)
            workspace = m->activeWorkspaceID();
        bool overview_enabled = overviews->overview_enabled(workspace);
        Vector2D vecSize = overview_enabled? overviews->get_vecsize(workspace) / overviews->get_scale(workspace) : m->vecSize;
        // If the monitor contains the point
        if (CBox{m->vecPosition, vecSize}.containsPoint(point)) {
            // Priority for last monitor
            if (m == last) {
                return last;
            }
            // Priority for monitor running overview
            if (overview_enabled) {
                mon = m;
            } else if (!mon) {
                mon = m;
            }
        }
    }
    if (mon)
        return mon;

    float      bestDistance = 0.f;
    PHLMONITOR pBestMon;

    for (auto const& m : compositor->m_vMonitors) {
        float dist = vecToRectDistanceSquared(point, m->vecPosition, m->vecPosition + m->vecSize);

        if (dist < bestDistance || !pBestMon) {
            bestDistance = dist;
            pBestMon     = m;
        }
    }

    if (!pBestMon) { // ?????
        Debug::log(WARN, "getMonitorFromVector no close mon???");
        return compositor->m_vMonitors.front();
    }

    return pBestMon;
}

static void hookRenderMonitor(void *thisptr, PHLMONITOR monitor) {

    WORKSPACEID workspace = monitor->activeSpecialWorkspaceID();
    if (!workspace)
        workspace = monitor->activeWorkspaceID();
    bool overview_enabled = overviews->overview_enabled(workspace);
    float scale = monitor->scale;
    if (overview_enabled) {
        monitor->scale *= overviews->get_scale(workspace);
    }
    ((origRenderMonitor)(g_pRenderMonitorHook->m_pOriginal))(thisptr, monitor);
    if (overview_enabled) {
        monitor->scale = scale;
    }
}


Overview::Overview() : initialized(false)
{
    // Hook bool CWindow::visibleOnMonitor(PHLMONITOR pMonitor)
    auto FNS1 = HyprlandAPI::findFunctionsByName(PHANDLE, "visibleOnMonitor");
    if (!FNS1.empty()) {
        g_pVisibleOnMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS1[0].address, (bool *)hookVisibleOnMonitor);
        if (g_pVisibleOnMonitorHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS3 = HyprlandAPI::findFunctionsByName(PHANDLE, "renderLayer");
    if (!FNS3.empty()) {
        g_pRenderLayerHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS3[0].address, (bool *)hookRenderLayer);
        if (g_pRenderLayerHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS5 = HyprlandAPI::findFunctionsByName(PHANDLE, "logicalBox");
    if (!FNS5.empty()) {
        g_pLogicalBoxHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS5[0].address, (bool *)hookLogicalBox);
        if (g_pLogicalBoxHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS6 = HyprlandAPI::findFunctionsByName(PHANDLE, "renderSoftwareCursorsFor");
    if (!FNS5.empty()) {
        g_pRenderSoftwareCursorsForHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS6[0].address, (bool *)hookRenderSoftwareCursorsFor);
        if (g_pRenderSoftwareCursorsForHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS7 =
        HyprlandAPI::findFunctionsByName(PHANDLE, "getMonitorFromVector");
    if (!FNS5.empty()) {
        g_pGetMonitorFromVectorHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS7[0].address, (bool *)hookGetMonitorFromVector);
        if (g_pGetMonitorFromVectorHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS8 = HyprlandAPI::findFunctionsByName(PHANDLE, "closestValid");
    if (!FNS5.empty()) {
        g_pClosestValidHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS8[0].address, (bool *)hookClosestValid);
        if (g_pClosestValidHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS2 = HyprlandAPI::findFunctionsByName(PHANDLE, "renderMonitor");
    if (!FNS2.empty()) {
        g_pRenderMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS2[0].address, (bool *)hookRenderMonitor);
        if (g_pRenderMonitorHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    auto FNS4 = HyprlandAPI::findFunctionsByName(PHANDLE, "getCursorPosForMonitor");
    if (!FNS4.empty()) {
        g_pGetCursorPosForMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, FNS4[0].address, (bool *)hookGetCursorPosForMonitor);
        if (g_pGetCursorPosForMonitorHook == nullptr) {
            return;
        }
    } else {
        return;
    }

    initialized = true;
}

Overview::~Overview()
{
    if (overview_enabled()) {
        disable_hooks();
    }

    if (g_pGetCursorPosForMonitorHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pGetCursorPosForMonitorHook);
        g_pGetCursorPosForMonitorHook = nullptr;
    }

    if (g_pRenderMonitorHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pRenderMonitorHook);
        g_pRenderMonitorHook = nullptr;
    }

    if (g_pClosestValidHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pClosestValidHook);
        g_pClosestValidHook = nullptr;
    }

    if (g_pGetMonitorFromVectorHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pGetMonitorFromVectorHook);
        g_pGetMonitorFromVectorHook = nullptr;
    }

    if (g_pRenderSoftwareCursorsForHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pRenderSoftwareCursorsForHook);
        g_pRenderSoftwareCursorsForHook = nullptr;
    }

    if (g_pLogicalBoxHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pLogicalBoxHook);
        g_pLogicalBoxHook = nullptr;
    }

    if (g_pRenderLayerHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pRenderLayerHook);
        g_pRenderLayerHook = nullptr;
    }

    if (g_pVisibleOnMonitorHook != nullptr) {
        /* bool success = */HyprlandAPI::removeFunctionHook(PHANDLE, g_pVisibleOnMonitorHook);
        g_pVisibleOnMonitorHook = nullptr;
    }

    initialized = false;
}

bool Overview::enable(WORKSPACEID workspace)
{
    if (!initialized)
        return false;
    if (!overview_enabled()) {
        if (!enable_hooks())
            return false;
    }
    workspaces[workspace].overview = true;
    return true;
}

void Overview::disable(WORKSPACEID workspace)
{
    if (!initialized)
        return;
    workspaces[workspace].overview = false;
    if (!overview_enabled()) {
        disable_hooks();
    }
}

bool Overview::overview_enabled(WORKSPACEID workspace) const
{
    if (!initialized)
        return false;
    auto enabled = workspaces.find(workspace);
    if (enabled != workspaces.end()) {
        return enabled->second.overview;
    }
    return false;
}

void Overview::set_vecsize(WORKSPACEID workspace, const Vector2D &size)
{
    auto enabled = workspaces.find(workspace);
    if (enabled != workspaces.end()) {
        enabled->second.vecsize = size;
    }
}

void Overview::set_scale(WORKSPACEID workspace, float scale)
{
    auto enabled = workspaces.find(workspace);
    if (enabled != workspaces.end()) {
        enabled->second.scale = scale;
    }
}

Vector2D Overview::get_vecsize(WORKSPACEID workspace) const
{
    auto enabled = workspaces.find(workspace);
    if (enabled != workspaces.end()) {
        return enabled->second.vecsize;
    }
    return {};
}

float Overview::get_scale(WORKSPACEID workspace) const
{
    auto enabled = workspaces.find(workspace);
    if (enabled != workspaces.end()) {
        return enabled->second.scale;
    }
    return 1.0f;
}

bool Overview::overview_enabled() const
{
    for (auto workspace : workspaces) {
        if (workspace.second.overview)
            return true;
    }
    return false;
}

bool Overview::enable_hooks()
{
    if (initialized &&
        g_pGetCursorPosForMonitorHook != nullptr && g_pGetCursorPosForMonitorHook->hook() &&
        g_pRenderMonitorHook != nullptr && g_pRenderMonitorHook->hook() &&
        g_pVisibleOnMonitorHook != nullptr && g_pVisibleOnMonitorHook->hook() &&
        g_pRenderLayerHook != nullptr && g_pRenderLayerHook->hook() &&
        g_pLogicalBoxHook != nullptr && g_pLogicalBoxHook->hook() &&
        g_pRenderSoftwareCursorsForHook != nullptr && g_pRenderSoftwareCursorsForHook->hook() &&
        g_pClosestValidHook != nullptr && g_pClosestValidHook->hook() &&
        g_pGetMonitorFromVectorHook != nullptr && g_pGetMonitorFromVectorHook->hook()) {
        return true;
    }
    return false;
}

void Overview::disable_hooks()
{
    if (!initialized)
        return;

    if (g_pGetCursorPosForMonitorHook != nullptr) g_pGetCursorPosForMonitorHook->unhook();
    if (g_pRenderMonitorHook != nullptr) g_pRenderMonitorHook->unhook();
    if (g_pVisibleOnMonitorHook != nullptr) g_pVisibleOnMonitorHook->unhook();
    if (g_pRenderLayerHook != nullptr) g_pRenderLayerHook->unhook();
    if (g_pLogicalBoxHook != nullptr) g_pLogicalBoxHook->unhook();
    if (g_pRenderSoftwareCursorsForHook != nullptr) g_pRenderSoftwareCursorsForHook->unhook();
    if (g_pClosestValidHook != nullptr) g_pClosestValidHook->unhook();
    if (g_pGetMonitorFromVectorHook != nullptr) g_pGetMonitorFromVectorHook->unhook();
}

