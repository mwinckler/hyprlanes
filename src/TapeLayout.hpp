#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Hyprlanes {

using WindowId = std::uintptr_t;

enum class Direction { Left, Right, Up, Down };

struct Box {
    double x, y, width, height;
};

struct Tile {
    WindowId id;
    double width;
    WindowId returnBefore = 0;
    WindowId returnAfter = 0;
    double returnX = 0.0;
};

struct Row {
    std::vector<Tile> tiles;
    double height;
    double offset = 0.0;
    WindowId focused = 0;
};

// Coordinates and camera offsets are fractions of the workspace's usable area.
class TapeLayout {
  public:
    void defaults(double width, double height);
    void add(WindowId id, WindowId after = 0);
    void remove(WindowId id);
    void focus(WindowId id, bool reveal = true);
    WindowId neighbor(WindowId id, Direction direction) const;
    void move(WindowId id, Direction direction, bool newRow = false);
    void drop(WindowId id, double x, double y);
    void swap(WindowId first, WindowId second);
    void resize(WindowId id, double width, double height);
    void scroll(WindowId id, double dx, double dy);
    void center(WindowId id);

    std::optional<Box> box(WindowId id) const;
    const std::vector<Row>& rows() const { return m_rows; }
    WindowId active() const { return m_active; }

  private:
    struct Position {
        std::size_t row, tile;
    };

    std::optional<Position> locate(WindowId id) const;
    void compact();
    void clampOffsets();

    std::vector<Row> m_rows;
    WindowId m_active = 0;
    double m_offsetY = 0.0;
    double m_defaultWidth = 0.5;
    double m_defaultHeight = 0.95;
};

} // namespace Hyprlanes
