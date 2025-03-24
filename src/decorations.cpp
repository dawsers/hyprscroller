#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/pass/BorderPassElement.hpp>
#include <hyprland/src/render/pass/TexPassElement.hpp>

#include "decorations.h"
#include "window.h"

#include <pango/pangocairo.h>

extern HANDLE PHANDLE;

// SelectionBorders
SelectionBorders::SelectionBorders(Window *window) : IHyprWindowDecoration(window->get_window()), window(window) {
    m_pWindow = window->get_window();
}

SelectionBorders::~SelectionBorders() {
}

SDecorationPositioningInfo SelectionBorders::getPositioningInfo() {
    // Avoid duplicating the border, we will draw over it
    m_seExtents = {{}, {}};

    SDecorationPositioningInfo info;
    info.priority       = 10000;
    info.policy         = DECORATION_POSITION_STICKY;
    info.desiredExtents = m_seExtents;
    info.reserved       = true;
    info.edges          = DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP;

    m_seReportedExtents = m_seExtents;
    return info;
}

void SelectionBorders::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedGeometry = reply.assignedGeometry;
}

CBox SelectionBorders::assignedBoxGlobal() {
    CBox box = m_bAssignedGeometry;
    box.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP, m_pWindow.lock()));

    const auto PWORKSPACE = m_pWindow->m_pWorkspace;

    if (!PWORKSPACE)
        return box;

    const auto WORKSPACEOFFSET = PWORKSPACE && !m_pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset->value() : Vector2D();
    return box.translate(WORKSPACEOFFSET);
}

void SelectionBorders::draw(PHLMONITOR pMonitor, float const& a) {
    if (doesntWantBorders())
        return;

    if (m_bAssignedGeometry.width < m_seExtents.topLeft.x + 1 || m_bAssignedGeometry.height < m_seExtents.topLeft.y + 1)
        return;

    CBox windowBox = assignedBoxGlobal().translate(-pMonitor->vecPosition + m_pWindow->m_vFloatingOffset).expand(-m_pWindow->getRealBorderSize()).scale(pMonitor->scale).round();

    if (windowBox.width < 1 || windowBox.height < 1)
        return;

    auto       grad     = window->get_border_color();
    const bool ANIMATED = m_pWindow->m_fBorderFadeAnimationProgress->isBeingAnimated();
    float      a1       = a * (ANIMATED ? m_pWindow->m_fBorderFadeAnimationProgress->value() : 1.f);

    if (m_pWindow->m_fBorderAngleAnimationProgress->enabled()) {
        grad.m_fAngle += m_pWindow->m_fBorderAngleAnimationProgress->value() * M_PI * 2;
        grad.m_fAngle = normalizeAngleRad(grad.m_fAngle);
    }

    int        borderSize = m_pWindow->getRealBorderSize();
    const auto ROUNDING   = m_pWindow->rounding() * pMonitor->scale;
    const auto ROUNDINGPOWER = m_pWindow->roundingPower();

    CBorderPassElement::SBorderData data;
    data.box           = windowBox;
    data.grad1         = grad;
    data.round         = ROUNDING;
    data.roundingPower = ROUNDINGPOWER;
    data.a             = a;
    data.borderSize    = borderSize;

    if (ANIMATED) {
        data.hasGrad2 = true;
        data.grad1    = m_pWindow->m_cRealBorderColorPrevious;
        data.grad2    = grad;
        data.lerp     = m_pWindow->m_fBorderFadeAnimationProgress->value();
    }

    g_pHyprRenderer->m_sRenderPass.add(makeShared<CBorderPassElement>(data));
}

eDecorationType SelectionBorders::getDecorationType() {
    return DECORATION_BORDER;
}

void SelectionBorders::updateWindow(PHLWINDOW) {
    auto borderSize = m_pWindow->getRealBorderSize();

    if (borderSize == m_iLastBorderSize)
        return;

    if (borderSize <= 0 && m_iLastBorderSize <= 0)
        return;

    m_iLastBorderSize = borderSize;

    g_pDecorationPositioner->repositionDeco(this);
}

void SelectionBorders::damageEntire() {
    if (!validMapped(m_pWindow))
        return;

    auto       surfaceBox   = m_pWindow->getWindowMainSurfaceBox();
    const auto ROUNDING     = m_pWindow->rounding();
    const auto ROUNDINGSIZE = ROUNDING - M_SQRT1_2 * ROUNDING + 2;
    const auto BORDERSIZE   = m_pWindow->getRealBorderSize() + 1;

    const auto PWINDOWWORKSPACE = m_pWindow->m_pWorkspace;
    if (PWINDOWWORKSPACE && PWINDOWWORKSPACE->m_vRenderOffset->isBeingAnimated() && !m_pWindow->m_bPinned)
        surfaceBox.translate(PWINDOWWORKSPACE->m_vRenderOffset->value());
    surfaceBox.translate(m_pWindow->m_vFloatingOffset);

    CBox surfaceBoxExpandedBorder = surfaceBox;
    surfaceBoxExpandedBorder.expand(BORDERSIZE);
    CBox surfaceBoxShrunkRounding = surfaceBox;
    surfaceBoxShrunkRounding.expand(-ROUNDINGSIZE);

    CRegion borderRegion(surfaceBoxExpandedBorder);
    borderRegion.subtract(surfaceBoxShrunkRounding);

    for (auto const& m : g_pCompositor->m_vMonitors) {
        if (!g_pHyprRenderer->shouldRenderWindow(m_pWindow.lock(), m)) {
            const CRegion monitorRegion({m->vecPosition, m->vecSize});
            borderRegion.subtract(monitorRegion);
        }
    }

    g_pHyprRenderer->damageRegion(borderRegion);
}

eDecorationLayer SelectionBorders::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

uint64_t SelectionBorders::getDecorationFlags() {
    return !doesntWantBorders() ? DECORATION_PART_OF_MAIN_WINDOW : 0;
}

std::string SelectionBorders::getDisplayName() {
    return "Border";
}

bool SelectionBorders::doesntWantBorders() {
    return m_pWindow->m_sWindowData.noBorder.valueOrDefault() || m_pWindow->m_bX11DoesntWantBorders || m_pWindow->getRealBorderSize() == 0;
}


// JumpDecoration
JumpDecoration::JumpDecoration(PHLWINDOW window, const std::string &label) : IHyprWindowDecoration(window) {
    m_pWindow = window;
    m_sLabel = label;
    m_iFrames = 0;
    m_pTexture = nullptr;
}

JumpDecoration::~JumpDecoration() {
}

SDecorationPositioningInfo JumpDecoration::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy = DECORATION_POSITION_STICKY;
    info.edges = DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP;
    return info;
}

void JumpDecoration::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedGeometry = reply.assignedGeometry;
}

CBox JumpDecoration::assignedBoxGlobal() {
    static auto *const *TEXTSCALE = (Hyprlang::FLOAT *const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:jump_labels_scale")->getDataStaticPtr();
    CBox box = m_bAssignedGeometry;
    box.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP, m_pWindow.lock()));
    if (box.w > box.h) {
        box.x += 0.5 * (box.w - box.h);
        box.w = box.h;
    } else {
        box.y += 0.5 * (box.h - box.w);
        box.h = box.w;
    }

    const double scale = **TEXTSCALE < 0.1 ? 0.1 : **TEXTSCALE > 1.0 ? 1.0 : **TEXTSCALE;
    box.scaleFromCenter(scale);

    const auto PWORKSPACE = m_pWindow->m_pWorkspace;

    if (!PWORKSPACE)
        return box;

    const auto WORKSPACEOFFSET = PWORKSPACE->m_vRenderOffset->value();
    return box.translate(WORKSPACEOFFSET);
}

void JumpDecoration::draw(PHLMONITOR pMonitor, float const& a) {
    CBox windowBox = assignedBoxGlobal().translate(-pMonitor->vecPosition).scale(pMonitor->scale).round();

    if (windowBox.width < 1 || windowBox.height < 1)
        return;
    
    const bool ANIMATED = m_pWindow->m_vRealPosition->isBeingAnimated() || m_pWindow->m_vRealSize->isBeingAnimated();

    if (m_pTexture.get() == nullptr) {
        static auto  FALLBACKFONT = CConfigValue<std::string>("misc:font_family");
        static auto const *TEXTFONTFAMILY = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:jump_labels_font")->getDataStaticPtr();
        static auto *const *TEXTCOL = (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:scroller:jump_labels_color")->getDataStaticPtr();
        const CHyprColor color = CHyprColor(**TEXTCOL);
        std::string font_family(*TEXTFONTFAMILY);
        if (font_family == "")
            font_family = *FALLBACKFONT;

        auto TEXTFONTSIZE = windowBox.width / m_sLabel.size();
        m_pTexture = g_pHyprOpenGL->renderText(m_sLabel, color, TEXTFONTSIZE, false, font_family, windowBox.width);
    }

    CTexPassElement::SRenderData data;
    data.tex = m_pTexture;
    data.box = windowBox;
    g_pHyprRenderer->m_sRenderPass.add(makeShared<CTexPassElement>(data));
}

eDecorationType JumpDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

void JumpDecoration::updateWindow(PHLWINDOW) {
}

void JumpDecoration::damageEntire() {
}

eDecorationLayer JumpDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

uint64_t JumpDecoration::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW;
}

std::string JumpDecoration::getDisplayName() {
    return "Overview";
}


