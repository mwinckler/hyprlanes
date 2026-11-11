#include "Hyprlanes.hpp"

#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/supplementary/DragController.hpp>
#include <hyprland/src/layout/target/Target.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>
#include <hyprland/src/managers/fullscreen/handler/FullscreenHandler.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/pointer/PointerController.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>

#include <charconv>
#include <cmath>
#include <sstream>
#include <utility>

namespace Hyprlanes {
namespace {

WindowId idOf(const SP<Layout::ITarget>& target) {
    return reinterpret_cast<WindowId>(target.get());
}

std::optional<Direction> parseDirection(std::string_view text) {
    if (text == "l" || text == "left") {
        return Direction::Left;
    }
    if (text == "r" || text == "right") {
        return Direction::Right;
    }
    if (text == "u" || text == "up") {
        return Direction::Up;
    }
    if (text == "d" || text == "down") {
        return Direction::Down;
    }
    return std::nullopt;
}

std::optional<double> number(std::string_view text) {
    if (text.starts_with('+')) {
        text.remove_prefix(1);
    }
    double result = 0.0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(result)) {
        return std::nullopt;
    }
    return result;
}

} // namespace

Algorithm::Algorithm() {
    auto& events = Event::bus()->m_events;
    m_focusListener = events.window.active.listen([this](PHLWINDOW window, Desktop::eFocusReason reason) {
        if (!window || !m_parent || window->m_workspace != m_parent->space()->workspace()) {
            return;
        }
        const auto id = idOf(window->layoutTarget());
        if (!m_targets.contains(id)) {
            return;
        }
        // Pointer motion remembers focus without dragging the camera under the pointer.
        m_layout.focus(id, !m_swiping && reason != Desktop::FOCUS_REASON_FFM);
        arrange();
    });

    m_swipeBeginListener = events.gesture.swipe.begin.listen([this](IPointer::SSwipeBeginEvent event, Event::SCallbackInfo& info) {
        static const auto fingers = CConfigValue<Config::INTEGER>("plugin:hyprlanes:gesture_fingers");
        if (info.cancelled || !activeWorkspace() || *fingers == 0 || event.fingers != static_cast<uint32_t>(*fingers) ||
            Fullscreen::controller()->hasFullscreen(m_parent->space()->workspace(), true)) {
            return;
        }
        m_swipeTarget = focused();
        const auto pointer = g_pInputManager->getMouseCoordsInternal();
        for (const auto& [id, weak] : m_targets) {
            if (const auto item = weak.lock(); item && item->position().containsPoint(pointer)) {
                m_swipeTarget = id;
                break;
            }
        }
        m_swiping = m_swipeTarget != 0;
        info.cancelled = m_swiping;
    });

    m_swipeUpdateListener = events.gesture.swipe.update.listen([this](IPointer::SSwipeUpdateEvent event, Event::SCallbackInfo& info) {
        if (!m_swiping) {
            return;
        }
        info.cancelled = true;
        const auto bounds = area();
        if (bounds.w <= 0 || bounds.h <= 0) {
            return;
        }
        static const auto natural = CConfigValue<Config::INTEGER>("input:touchpad:natural_scroll");
        static const auto sensitivity = CConfigValue<Config::FLOAT>("plugin:hyprlanes:gesture_sensitivity");
        const double scale = *sensitivity * (*natural ? -1.0 : 1.0);
        m_layout.scroll(m_swipeTarget, scale * event.delta.x / bounds.w, scale * event.delta.y / bounds.h);
        arrange(true);
    });

    m_swipeEndListener = events.gesture.swipe.end.listen([this](IPointer::SSwipeEndEvent, Event::SCallbackInfo& info) {
        if (m_swiping) {
            info.cancelled = true;
            m_swiping = false;
            m_swipeTarget = 0;
        }
    });
}

CBox Algorithm::area() const {
    if (!m_parent || !m_parent->space() || !m_parent->space()->workspace() || !m_parent->space()->workspace()->m_monitor) {
        return {};
    }
    return m_parent->space()->workArea();
}

bool Algorithm::activeWorkspace() const {
    const auto monitor = Desktop::focusState()->monitor();
    if (!monitor || !m_parent || !m_parent->space()) {
        return false;
    }
    const auto workspace = monitor->m_activeSpecialWorkspace ? monitor->m_activeSpecialWorkspace : monitor->m_activeWorkspace;
    return workspace == m_parent->space()->workspace();
}

WindowId Algorithm::focused() const {
    if (const auto window = Desktop::focusState()->window()) {
        const auto id = idOf(window->layoutTarget());
        if (m_targets.contains(id)) {
            return id;
        }
    }
    return m_layout.active();
}

SP<Layout::ITarget> Algorithm::target(WindowId id) const {
    const auto it = m_targets.find(id);
    return it == m_targets.end() ? nullptr : it->second.lock();
}

void Algorithm::focusWindow(WindowId id) {
    const auto item = target(id);
    if (!item || !item->window()) {
        return;
    }
    m_layout.focus(id);
    arrange();
    Desktop::focusState()->fullWindowFocus(item->window(), Desktop::FOCUS_REASON_KEYBIND);
    Pointer::pointerController()->warpTo(item->window()->middle());
}

void Algorithm::refreshDefaults() {
    static const auto width = CConfigValue<Config::FLOAT>("plugin:hyprlanes:column_width");
    static const auto height = CConfigValue<Config::FLOAT>("plugin:hyprlanes:row_height");
    m_layout.defaults(*width, *height);
}

void Algorithm::newTarget(SP<Layout::ITarget> item) {
    refreshDefaults();
    const auto after = focused();
    const auto id = idOf(item);
    m_targets[id] = item;
    m_layout.add(id, after);
    if (g_layoutManager->dragController()->wasDraggingWindow()) {
        const auto bounds = area();
        if (bounds.w > 0 && bounds.h > 0) {
            const auto pointer = g_pInputManager->getMouseCoordsInternal();
            m_layout.drop(id, (pointer.x - bounds.x) / bounds.w, (pointer.y - bounds.y) / bounds.h);
        }
    }
    arrange();
}

void Algorithm::movedTarget(SP<Layout::ITarget> item, std::optional<Vector2D> focalPoint) {
    newTarget(item);
    const auto bounds = area();
    if (focalPoint && bounds.w > 0 && bounds.h > 0) {
        m_layout.drop(idOf(item), (focalPoint->x - bounds.x) / bounds.w, (focalPoint->y - bounds.y) / bounds.h);
        arrange();
    }
}

void Algorithm::removeTarget(SP<Layout::ITarget> item) {
    m_layout.remove(idOf(item));
    m_targets.erase(idOf(item));
    arrange();
}

void Algorithm::resizeTarget(const Vector2D& delta, SP<Layout::ITarget> item, Layout::eRectCorner corner) {
    const auto bounds = area();
    const auto tile = m_layout.box(idOf(item));
    if (!tile || bounds.w <= 0 || bounds.h <= 0) {
        return;
    }
    const double dx = Layout::edgeLeft(corner) ? -delta.x : delta.x;
    const double dy = Layout::edgeTop(corner) ? -delta.y : delta.y;
    m_layout.resize(idOf(item), tile->width + dx / bounds.w, tile->height + dy / bounds.h);
    arrange(true);
}

void Algorithm::recalculate(Layout::eRecalculateReason reason) {
    if (Layout::isHardRecalculateReason(reason)) {
        m_layout.focus(focused());
    }
    arrange();
}

void Algorithm::arrange(bool instant) {
    const auto bounds = area();
    if (bounds.w <= 0 || bounds.h <= 0 || std::exchange(m_arranging, true)) {
        return;
    }
    Hyprutils::Utils::CScopeGuard guard([this] { m_arranging = false; });
    if (Fullscreen::controller()->hasFullscreen(m_parent->space()->workspace(), true)) {
        m_defaultFullscreenHandler->syncTargetSizeAndPosition();
        return;
    }
    for (const auto& row : m_layout.rows()) {
        for (const auto& tile : row.tiles) {
            const auto item = target(tile.id);
            if (!item) {
                continue;
            }
            const auto box = *m_layout.box(tile.id);
            item->setPositionGlobal(
                CBox{bounds.x + box.x * bounds.w, bounds.y + box.y * bounds.h, box.width * bounds.w, box.height * bounds.h});
            if (instant) {
                item->warpPositionSize();
            }
        }
    }
}

void Algorithm::swapTargets(SP<Layout::ITarget> first, SP<Layout::ITarget> second) {
    const auto a = idOf(first);
    const auto b = idOf(second);
    const bool hasA = m_targets.contains(a);
    const bool hasB = m_targets.contains(b);
    m_layout.swap(a, b);
    if (hasA && !hasB) {
        m_targets.erase(a);
        m_targets[b] = second;
    } else if (hasB && !hasA) {
        m_targets.erase(b);
        m_targets[a] = first;
    }
    arrange();
}

void Algorithm::moveTargetInDirection(SP<Layout::ITarget> item, Math::eDirection direction, bool silent) {
    Direction move;
    switch (direction) {
        case Math::DIRECTION_LEFT:
            move = Direction::Left;
            break;
        case Math::DIRECTION_RIGHT:
            move = Direction::Right;
            break;
        case Math::DIRECTION_UP:
            move = Direction::Up;
            break;
        case Math::DIRECTION_DOWN:
            move = Direction::Down;
            break;
        default:
            return;
    }
    refreshDefaults();
    m_layout.move(idOf(item), move);
    arrange();
    if (!silent) {
        focusWindow(idOf(item));
    }
}

SP<Layout::ITarget> Algorithm::getNextCandidate(SP<Layout::ITarget> old) {
    if (m_layout.active() != idOf(old)) {
        return target(m_layout.active());
    }
    const auto next = m_layout.neighbor(idOf(old), Direction::Right);
    if (next != idOf(old)) {
        return target(next);
    }
    for (const auto& row : m_layout.rows()) {
        for (const auto& tile : row.tiles) {
            if (tile.id != idOf(old)) {
                return target(tile.id);
            }
        }
    }
    return nullptr;
}

std::optional<Vector2D> Algorithm::predictSizeForNewTarget() {
    static const auto width = CConfigValue<Config::FLOAT>("plugin:hyprlanes:column_width");
    static const auto height = CConfigValue<Config::FLOAT>("plugin:hyprlanes:row_height");
    const auto bounds = area();
    const auto active = m_layout.box(focused());
    return Vector2D{bounds.w * *width, bounds.h * (active ? active->height : *height)};
}

Config::ErrorResult Algorithm::layoutMsg(const std::string_view& message) {
    const auto invalid = [](const std::string& text) {
        return Config::configError("hyprlanes: " + text, Config::eConfigErrorLevel::ERROR, Config::eConfigErrorCode::INVALID_ARGUMENT);
    };
    std::istringstream input{std::string(message)};
    std::string command, arg, second, extra;
    input >> command >> arg >> second >> extra;
    if (!extra.empty() || (command != "scroll" && !second.empty())) {
        return invalid("Too many arguments.");
    }
    const auto id = focused();
    if (command == "focus" || command == "move" || command == "newrow") {
        const auto direction = parseDirection(arg);
        if (!direction || (command == "newrow" && *direction != Direction::Up && *direction != Direction::Down)) {
            return invalid("Use left/right/up/down, or up/down for newrow.");
        }
        if (command == "focus") {
            focusWindow(m_layout.neighbor(id, *direction));
        } else {
            refreshDefaults();
            m_layout.move(id, *direction, command == "newrow");
            focusWindow(id);
        }
    } else if (command == "width" || command == "height") {
        const auto value = number(arg);
        if (!value) {
            return invalid("Specify a finite fraction, such as 0.5 or +0.1.");
        }
        if (const auto box = m_layout.box(id)) {
            const bool relative = arg.starts_with('+') || arg.starts_with('-');
            const double size = *value + (relative ? (command == "width" ? box->width : box->height) : 0.0);
            m_layout.resize(id, command == "width" ? size : box->width, command == "height" ? size : box->height);
            arrange();
        }
    } else if (command == "scroll") {
        const auto dx = number(arg);
        const auto dy = number(second);
        if (!dx || !dy) {
            return invalid("Specify horizontal and vertical viewport fractions, such as scroll 0.25 0.");
        }
        m_layout.scroll(id, *dx, *dy);
        arrange();
    } else if (command == "center" && arg.empty()) {
        m_layout.center(id);
        arrange();
    } else {
        return invalid("Use focus, move, newrow, width, height, scroll, or center.");
    }
    return {};
}

} // namespace Hyprlanes
