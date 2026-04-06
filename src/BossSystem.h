#pragma once

#include <cstdint>

#include "AssetManager.h"
#include "BulletSystem.h"
#include "Components.h"
#include "ECS.h"

class BossSystem
{
public:
    BossSystem();

    void spawnBoss(Registry& reg, AssetManager& assets, Vector2 worldCenter);
    void update(Registry& reg,
                std::uint32_t playerEntityId,
                BulletSystem& bullets,
                AssetManager& assets,
                float dt);
    bool isDefeated(Registry& reg);
    bool isActive() const { return active; }
    void reset();

private:
    bool active;
    std::uint32_t bossEntityId;
    float spiralAngle;
    bool alternateCharge;
};
