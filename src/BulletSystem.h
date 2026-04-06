#pragma once

#include <cstdint>
#include <vector>

#include "Components.h"
#include "ECS.h"
#include "PhysicsSystem.h"

class BulletSystem
{
public:
    void update(Registry& reg, const std::vector<PhysicsSystem::CollisionEvent>& collisions, float dt);
    std::uint32_t spawnBullet(Registry& reg,
                              Vector2 worldPos,
                              float angleDegrees,
                              float speed,
                              float damage,
                              bool fromPlayer,
                              SDL_Texture* texture,
                              float maxRange = 900.0f);
};
