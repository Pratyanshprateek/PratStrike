#pragma once

#include <cstdint>

#include "AssetManager.h"
#include "BulletSystem.h"
#include "Components.h"
#include "ECS.h"
#include "Quadtree.h"

class AISystem
{
public:
    void update(Registry& reg,
                Quadtree& worldTree,
                std::uint32_t playerEntityId,
                int difficultyIndex,
                BulletSystem& bullets,
                AssetManager& assets,
                float dt);
};
