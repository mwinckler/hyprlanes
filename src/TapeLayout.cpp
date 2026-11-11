#include "TapeLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace Hyprlanes {

void TapeLayout::defaults(double width, double height) {
    m_defaultWidth = std::clamp(width, 0.1, 1.0);
    m_defaultHeight = std::clamp(height, 0.1, 1.0);
}

std::optional<TapeLayout::Position> TapeLayout::locate(WindowId id) const {
    for (std::size_t r = 0; r < m_rows.size(); ++r) {
        const auto& tiles = m_rows[r].tiles;
        for (std::size_t t = 0; t < tiles.size(); ++t) {
            if (tiles[t].id == id) {
                return Position{r, t};
            }
        }
    }
    return std::nullopt;
}

void TapeLayout::add(WindowId id, WindowId after) {
    if (locate(id)) {
        return;
    }
    const auto position = locate(after ? after : m_active);
    if (m_rows.empty()) {
        m_rows.push_back({{}, m_defaultHeight});
    }
    auto& row = m_rows[position ? position->row : m_rows.size() - 1];
    const auto index = position ? position->tile + 1 : row.tiles.size();
    row.tiles.insert(row.tiles.begin() + index, {id, m_defaultWidth});
    focus(id);
}

void TapeLayout::remove(WindowId id) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    auto& row = m_rows[position->row];
    row.tiles.erase(row.tiles.begin() + position->tile);
    if (row.focused == id) {
        row.focused = row.tiles.empty() ? 0 : row.tiles[std::min(position->tile, row.tiles.size() - 1)].id;
    }
    if (m_active == id) {
        m_active = row.focused;
    }
    compact();
    focus(m_active);
}

void TapeLayout::compact() {
    std::erase_if(m_rows, [](const Row& row) { return row.tiles.empty(); });
    for (auto& row : m_rows) {
        if (std::ranges::none_of(row.tiles, [&](const Tile& tile) { return tile.id == row.focused; })) {
            row.focused = row.tiles.front().id;
        }
    }
    if (!locate(m_active)) {
        m_active = m_rows.empty() ? 0 : m_rows.back().focused;
    }
    clampOffsets();
}

void TapeLayout::clampOffsets() {
    if (m_rows.empty()) {
        m_offsetY = 0.0;
        return;
    }
    double height = 0.0;
    for (auto& row : m_rows) {
        const double width =
            std::accumulate(row.tiles.begin(), row.tiles.end(), 0.0, [](double sum, const Tile& tile) { return sum + tile.width; });
        row.offset = std::clamp(row.offset, 0.0, std::max(0.0, width - 1.0));
        height += row.height;
    }
    // Allow the first and last lanes to reach the vertical center.
    const double minOffset = (m_rows.front().height - 1.0) / 2.0;
    const double maxOffset = height - m_rows.back().height + (m_rows.back().height - 1.0) / 2.0;
    m_offsetY = std::clamp(m_offsetY, minOffset, maxOffset);
}

std::optional<Box> TapeLayout::box(WindowId id) const {
    double y = -m_offsetY;
    for (const auto& row : m_rows) {
        double x = -row.offset;
        for (const auto& tile : row.tiles) {
            if (tile.id == id) {
                return Box{x, y, tile.width, row.height};
            }
            x += tile.width;
        }
        y += row.height;
    }
    return std::nullopt;
}

void TapeLayout::focus(WindowId id, bool reveal) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    m_active = id;
    auto& row = m_rows[position->row];
    row.focused = id;
    if (reveal) {
        const auto bounds = *box(id);
        row.offset += bounds.x < 0.0 ? bounds.x : std::max(0.0, bounds.x + bounds.width - 1.0);
        m_offsetY += bounds.y + bounds.height / 2.0 - 0.5;
        clampOffsets();
    }
}

WindowId TapeLayout::neighbor(WindowId id, Direction direction) const {
    const auto position = locate(id);
    if (!position) {
        return m_active;
    }
    const auto& row = m_rows[position->row];
    switch (direction) {
        case Direction::Left:
            return position->tile > 0 ? row.tiles[position->tile - 1].id : id;
        case Direction::Right:
            return position->tile + 1 < row.tiles.size() ? row.tiles[position->tile + 1].id : id;
        case Direction::Up:
            return position->row > 0 ? m_rows[position->row - 1].focused : id;
        case Direction::Down:
            return position->row + 1 < m_rows.size() ? m_rows[position->row + 1].focused : id;
    }
    return id;
}

void TapeLayout::move(WindowId id, Direction direction, bool newRow) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    if (direction == Direction::Left || direction == Direction::Right) {
        const auto other = locate(neighbor(id, direction));
        std::swap(m_rows[position->row].tiles[position->tile], m_rows[other->row].tiles[other->tile]);
        focus(id);
        return;
    }

    const Tile tile = m_rows[position->row].tiles[position->tile];
    auto& source = m_rows[position->row];
    auto moved = tile;
    moved.returnBefore = position->tile + 1 < source.tiles.size() ? source.tiles[position->tile + 1].id : 0;
    moved.returnAfter = position->tile > 0 ? source.tiles[position->tile - 1].id : 0;
    moved.returnX = box(id)->x;
    source.tiles.erase(source.tiles.begin() + position->tile);
    if (source.focused == id) {
        source.focused = source.tiles.empty() ? 0 : source.tiles[std::min(position->tile, source.tiles.size() - 1)].id;
    }

    const bool up = direction == Direction::Up;
    const bool atEdge = up ? position->row == 0 : position->row + 1 == m_rows.size();
    std::size_t destination;
    if (newRow || atEdge) {
        destination = position->row + (up ? 0 : 1);
        m_rows.insert(m_rows.begin() + destination, Row{{}, source.height});
    } else {
        destination = up ? position->row - 1 : position->row + 1;
    }
    auto& row = m_rows[destination];
    const auto before = std::ranges::find(row.tiles, tile.returnBefore, &Tile::id);
    const auto after = std::ranges::find(row.tiles, tile.returnAfter, &Tile::id);
    const bool returning = before != row.tiles.end() || after != row.tiles.end();
    auto insertion = before;
    if (insertion == row.tiles.end()) {
        const auto anchor = after != row.tiles.end() ? after : std::ranges::find(row.tiles, row.focused, &Tile::id);
        insertion = anchor == row.tiles.end() ? anchor : anchor + 1;
    }
    row.tiles.insert(insertion, moved);
    if (returning) {
        row.offset += box(id)->x - tile.returnX;
    }
    compact();
    focus(id);
}

void TapeLayout::drop(WindowId id, double x, double y) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    std::size_t destination = 0;
    double top = -m_offsetY;
    double distance = std::numeric_limits<double>::max();
    for (std::size_t r = 0; r < m_rows.size(); ++r) {
        const auto height = m_rows[r].height;
        const double gap = std::abs(y - std::clamp(y, top, top + height));
        if (gap < distance) {
            destination = r;
            distance = gap;
        }
        top += height;
    }
    auto& source = m_rows[position->row].tiles;
    const auto tile = source[position->tile];
    source.erase(source.begin() + position->tile);
    auto& row = m_rows[destination];
    double left = -row.offset;
    auto insertion = row.tiles.begin();
    while (insertion != row.tiles.end() && x > left + insertion->width / 2.0) {
        left += insertion->width;
        ++insertion;
    }
    row.tiles.insert(insertion, tile);
    compact();
    focus(id);
}

void TapeLayout::swap(WindowId first, WindowId second) {
    const auto a = locate(first);
    const auto b = locate(second);
    if (a && b) {
        std::swap(m_rows[a->row].tiles[a->tile], m_rows[b->row].tiles[b->tile]);
    } else if (a) {
        m_rows[a->row].tiles[a->tile].id = second;
    } else if (b) {
        m_rows[b->row].tiles[b->tile].id = first;
    }
    for (auto& row : m_rows) {
        if (row.focused == first) {
            row.focused = second;
        } else if (row.focused == second) {
            row.focused = first;
        }
    }
    if (m_active == first) {
        m_active = second;
    } else if (m_active == second) {
        m_active = first;
    }
    clampOffsets();
}

void TapeLayout::resize(WindowId id, double width, double height) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    auto& row = m_rows[position->row];
    row.tiles[position->tile].width = std::clamp(width, 0.1, 1.0);
    row.height = std::clamp(height, 0.1, 1.0);
    clampOffsets();
    focus(id);
}

void TapeLayout::scroll(WindowId id, double dx, double dy) {
    const auto position = locate(id);
    if (position) {
        m_rows[position->row].offset += dx;
    }
    m_offsetY += dy;
    clampOffsets();
}

void TapeLayout::center(WindowId id) {
    const auto position = locate(id);
    if (!position) {
        return;
    }
    const auto bounds = *box(id);
    m_rows[position->row].offset += bounds.x + bounds.width / 2.0 - 0.5;
    m_offsetY += bounds.y + bounds.height / 2.0 - 0.5;
    clampOffsets();
}

} // namespace Hyprlanes
