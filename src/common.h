#ifndef SCROLLER_COMMON_H
#define SCROLLER_COMMON_H

#include <hyprutils/math/Vector2D.hpp>

using Hyprutils::Math::Vector2D;

struct Box {
    Box() : x(0), y(0), w(0), h(0) {}
    Box(double x_, double y_, double w_, double h_)
        : x(x_), y(y_), w(w_), h(h_) {}
    Box(Vector2D pos, Vector2D size)
        : x(pos.x), y(pos.y), w(size.x), h(size.y) {}
    Box(const Box &box)
        : x(box.x), y(box.y), w(box.w), h(box.h) {}
    Box& operator=(const Box&) = default;

    void set_size(double w_, double h_) {
        w = w_;
        h = h_;
    }
    void set_pos(double x_, double y_) {
        x = x_;
        y = y_;
    }
    bool operator == (const Box &box) {
        if (box.x == x && box.y == y && box.w == w && box.h == h)
            return true;
        return false;
    }
    bool contains_point(const Vector2D &vec) const {
        if (vec.x >= x && vec.x < x + w && vec.y >= y && vec.y < y + h)
            return true;
        return false;
    }

    double x, y, w, h;
};

#endif // SCROLLER_COMMON_H
