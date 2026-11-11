#pragma once

#include "TapeLayout.hpp"

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>

#include <unordered_map>

namespace Hyprlanes {

class Algorithm : public Layout::ITiledAlgorithm {
  public:
    Algorithm();

    void newTarget(SP<Layout::ITarget> target) override;
    void movedTarget(SP<Layout::ITarget> target, std::optional<Vector2D> focalPoint) override;
    void removeTarget(SP<Layout::ITarget> target) override;
    void resizeTarget(const Vector2D& delta, SP<Layout::ITarget> target, Layout::eRectCorner corner) override;
    void recalculate(Layout::eRecalculateReason reason = Layout::RECALCULATE_REASON_UNKNOWN) override;
    void swapTargets(SP<Layout::ITarget> first, SP<Layout::ITarget> second) override;
    void moveTargetInDirection(SP<Layout::ITarget> target, Math::eDirection direction, bool silent) override;
    SP<Layout::ITarget> getNextCandidate(SP<Layout::ITarget> old) override;
    Config::ErrorResult layoutMsg(const std::string_view& message) override;
    std::optional<Vector2D> predictSizeForNewTarget() override;

  private:
    CBox area() const;
    WindowId focused() const;
    SP<Layout::ITarget> target(WindowId id) const;
    void refreshDefaults();
    void focusWindow(WindowId id);
    void arrange(bool instant = false);
    bool activeWorkspace() const;

    TapeLayout m_layout;
    std::unordered_map<WindowId, WP<Layout::ITarget>> m_targets;
    bool m_arranging = false;
    bool m_swiping = false;
    WindowId m_swipeTarget = 0;
    CHyprSignalListener m_focusListener, m_swipeBeginListener, m_swipeUpdateListener, m_swipeEndListener;
};

} // namespace Hyprlanes
