#include "Quadtree.h"

namespace
{
    bool rectIntersectsCircle(const Rect& rect, const Vector2& center, float radius)
    {
        const float closestX = clamp(center.x, rect.x, rect.x + rect.w);
        const float closestY = clamp(center.y, rect.y, rect.y + rect.h);
        const float dx = center.x - closestX;
        const float dy = center.y - closestY;
        return ((dx * dx) + (dy * dy)) <= (radius * radius);
    }
}

Quadtree::Quadtree(Rect boundaryRect, int nodeCapacity)
    : boundary(boundaryRect),
      capacity(nodeCapacity),
      divided(false)
{
}

void Quadtree::clear()
{
    entries.clear();
    nw.reset();
    ne.reset();
    sw.reset();
    se.reset();
    divided = false;
}

bool Quadtree::insert(std::uint32_t entityId, Rect bounds)
{
    return insertInternal(QtEntry{entityId, bounds});
}

void Quadtree::query(Rect area, std::vector<std::uint32_t>& out)
{
    std::unordered_set<std::uint32_t> uniqueIds;
    queryInternal(area, uniqueIds);
    out.insert(out.end(), uniqueIds.begin(), uniqueIds.end());
}

void Quadtree::queryCircle(Vector2 center, float radius, std::vector<std::uint32_t>& out)
{
    const Rect circleBounds(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
    std::vector<QtEntry> candidateEntries;
    std::unordered_set<std::uint32_t> seen;
    queryEntriesInternal(circleBounds, candidateEntries, seen);

    for (const QtEntry& entry : candidateEntries)
    {
        if (rectIntersectsCircle(entry.bounds, center, radius))
        {
            out.push_back(entry.entityId);
        }
    }
}

void Quadtree::queryPoint(Vector2 point, std::vector<std::uint32_t>& out)
{
    std::unordered_set<std::uint32_t> uniqueIds;
    queryPointInternal(point, uniqueIds);
    out.insert(out.end(), uniqueIds.begin(), uniqueIds.end());
}

void Quadtree::subdivide()
{
    const float halfW = boundary.w * 0.5f;
    const float halfH = boundary.h * 0.5f;

    nw = std::make_unique<Quadtree>(Rect(boundary.x, boundary.y, halfW, halfH), capacity);
    ne = std::make_unique<Quadtree>(Rect(boundary.x + halfW, boundary.y, halfW, halfH), capacity);
    sw = std::make_unique<Quadtree>(Rect(boundary.x, boundary.y + halfH, halfW, halfH), capacity);
    se = std::make_unique<Quadtree>(Rect(boundary.x + halfW, boundary.y + halfH, halfW, halfH), capacity);
    divided = true;
}

bool Quadtree::childCanFullyContain(const Quadtree& child, const Rect& bounds) const
{
    const Rect& area = child.boundary;
    return bounds.x >= area.x &&
           bounds.y >= area.y &&
           (bounds.x + bounds.w) <= (area.x + area.w) &&
           (bounds.y + bounds.h) <= (area.y + area.h);
}

bool Quadtree::insertInternal(QtEntry entry)
{
    if (!boundary.intersects(entry.bounds) && !boundary.contains(entry.bounds.center()))
    {
        return false;
    }

    if (!divided && static_cast<int>(entries.size()) < capacity)
    {
        entries.push_back(entry);
        return true;
    }

    if (!divided)
    {
        subdivide();

        const std::vector<QtEntry> existingEntries = entries;
        entries.clear();
        for (const QtEntry& existing : existingEntries)
        {
            bool movedToChild = false;
            if (childCanFullyContain(*nw, existing.bounds))
            {
                movedToChild = nw->insertInternal(existing);
            }
            else if (childCanFullyContain(*ne, existing.bounds))
            {
                movedToChild = ne->insertInternal(existing);
            }
            else if (childCanFullyContain(*sw, existing.bounds))
            {
                movedToChild = sw->insertInternal(existing);
            }
            else if (childCanFullyContain(*se, existing.bounds))
            {
                movedToChild = se->insertInternal(existing);
            }

            if (!movedToChild)
            {
                entries.push_back(existing);
            }
        }
    }

    if (childCanFullyContain(*nw, entry.bounds))
    {
        return nw->insertInternal(entry);
    }
    if (childCanFullyContain(*ne, entry.bounds))
    {
        return ne->insertInternal(entry);
    }
    if (childCanFullyContain(*sw, entry.bounds))
    {
        return sw->insertInternal(entry);
    }
    if (childCanFullyContain(*se, entry.bounds))
    {
        return se->insertInternal(entry);
    }

    if (boundary.intersects(entry.bounds) || boundary.contains(entry.bounds.center()))
    {
        entries.push_back(entry);
        return true;
    }

    return false;
}

void Quadtree::queryInternal(const Rect& area, std::unordered_set<std::uint32_t>& out) const
{
    if (!boundary.intersects(area))
    {
        return;
    }

    for (const QtEntry& entry : entries)
    {
        if (entry.bounds.intersects(area))
        {
            out.insert(entry.entityId);
        }
    }

    if (!divided)
    {
        return;
    }

    nw->queryInternal(area, out);
    ne->queryInternal(area, out);
    sw->queryInternal(area, out);
    se->queryInternal(area, out);
}

void Quadtree::queryEntriesInternal(const Rect& area, std::vector<QtEntry>& out, std::unordered_set<std::uint32_t>& seen) const
{
    if (!boundary.intersects(area))
    {
        return;
    }

    for (const QtEntry& entry : entries)
    {
        if (entry.bounds.intersects(area) && seen.insert(entry.entityId).second)
        {
            out.push_back(entry);
        }
    }

    if (!divided)
    {
        return;
    }

    nw->queryEntriesInternal(area, out, seen);
    ne->queryEntriesInternal(area, out, seen);
    sw->queryEntriesInternal(area, out, seen);
    se->queryEntriesInternal(area, out, seen);
}

void Quadtree::queryPointInternal(const Vector2& point, std::unordered_set<std::uint32_t>& out) const
{
    if (!boundary.contains(point))
    {
        return;
    }

    for (const QtEntry& entry : entries)
    {
        if (entry.bounds.contains(point))
        {
            out.insert(entry.entityId);
        }
    }

    if (!divided)
    {
        return;
    }

    nw->queryPointInternal(point, out);
    ne->queryPointInternal(point, out);
    sw->queryPointInternal(point, out);
    se->queryPointInternal(point, out);
}
