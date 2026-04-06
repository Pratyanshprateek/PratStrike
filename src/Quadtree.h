#pragma once

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

#include "Math.h"

struct QtEntry
{
    std::uint32_t entityId;
    Rect bounds;
};

class Quadtree
{
public:
    Quadtree(Rect boundary, int capacity = 8);

    void clear();
    bool insert(std::uint32_t entityId, Rect bounds);
    void query(Rect area, std::vector<std::uint32_t>& out);
    void queryCircle(Vector2 center, float radius, std::vector<std::uint32_t>& out);
    void queryPoint(Vector2 point, std::vector<std::uint32_t>& out);

private:
    void subdivide();
    bool childCanFullyContain(const Quadtree& child, const Rect& bounds) const;
    bool insertInternal(QtEntry entry);
    void queryInternal(const Rect& area, std::unordered_set<std::uint32_t>& out) const;
    void queryEntriesInternal(const Rect& area, std::vector<QtEntry>& out, std::unordered_set<std::uint32_t>& seen) const;
    void queryPointInternal(const Vector2& point, std::unordered_set<std::uint32_t>& out) const;

    Rect boundary;
    int capacity;
    bool divided;
    std::vector<QtEntry> entries;
    std::unique_ptr<Quadtree> nw;
    std::unique_ptr<Quadtree> ne;
    std::unique_ptr<Quadtree> sw;
    std::unique_ptr<Quadtree> se;
};
