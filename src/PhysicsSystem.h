#pragma once

#include <cstdint>
#include <vector>

#include "Components.h"
#include "ECS.h"
#include "Quadtree.h"

class PhysicsSystem
{
public:
    struct CollisionEvent
    {
        std::uint32_t a;
        std::uint32_t b;
    };

    PhysicsSystem();

    void update(Registry& reg, float dt);
    const std::vector<CollisionEvent>& getCollisions() const;
    Quadtree& getQuadtree();
    const Quadtree& getQuadtree() const;
    void clearCollisions();

private:
    static Rect buildBounds(const TransformComponent& transform, const ColliderComponent& collider);

    Quadtree worldTree;
    std::vector<CollisionEvent> collisions;
};
