#include "PhysicsSystem.h"

#include <algorithm>
#include <unordered_set>

#include "GameFeel.h"

namespace
{
    constexpr float WORLD_WIDTH = 2560.0f;
    constexpr float WORLD_HEIGHT = 1440.0f;

    std::uint64_t makePairKey(std::uint32_t a, std::uint32_t b)
    {
        const std::uint32_t lo = (a < b) ? a : b;
        const std::uint32_t hi = (a < b) ? b : a;
        return (static_cast<std::uint64_t>(lo) << 32U) | static_cast<std::uint64_t>(hi);
    }
}

PhysicsSystem::PhysicsSystem()
    : worldTree(Rect(0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT), 8)
{
}

void PhysicsSystem::update(Registry& reg, float dt)
{
    collisions.clear();

    for (const std::uint32_t entityId : reg.view<TransformComponent>())
    {
        TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
        transform.prevPosition = transform.position;
        transform.prevRotation = transform.rotation;
    }

    bool hitstopActive = false;
    for (const std::uint32_t entityId : reg.view<HitstopComponent>())
    {
        HitstopComponent& hitstop = reg.getComponent<HitstopComponent>(entityId);
        if (hitstop.timer > 0.0f)
        {
            hitstop.timer = std::max(0.0f, hitstop.timer - dt);
        }

        if (hitstop.timer > 0.0f && isWorldHitstopEntity(entityId))
        {
            hitstopActive = true;
        }
    }

    if (hitstopActive)
    {
        return;
    }

    for (const std::uint32_t entityId : reg.view<TransformComponent, VelocityComponent>())
    {
        TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
        VelocityComponent& velocity = reg.getComponent<VelocityComponent>(entityId);

        const float frictionFactor = clamp(1.0f - (velocity.friction * 60.0f * dt), 0.0f, 1.0f);
        velocity.velocity *= frictionFactor;

        const float currentSpeed = velocity.velocity.length();
        if (currentSpeed > velocity.maxSpeed && currentSpeed > math::EPSILON)
        {
            velocity.velocity = velocity.velocity.normalized() * velocity.maxSpeed;
        }

        if (velocity.velocity.lengthSq() <= math::EPSILON && velocity.speed > 0.0f)
        {
            velocity.speed *= frictionFactor;
            velocity.speed = clamp(velocity.speed, 0.0f, velocity.maxSpeed);
            velocity.velocity = Vector2(0.0f, -1.0f).rotated(transform.rotation) * velocity.speed;
        }

        transform.position += velocity.velocity * dt;

        if (transform.position.x < 0.0f)
        {
            transform.position.x = 0.0f;
            velocity.velocity.x = -velocity.velocity.x * 0.3f;
        }
        else if (transform.position.x > WORLD_WIDTH)
        {
            transform.position.x = WORLD_WIDTH;
            velocity.velocity.x = -velocity.velocity.x * 0.3f;
        }

        if (transform.position.y < 0.0f)
        {
            transform.position.y = 0.0f;
            velocity.velocity.y = -velocity.velocity.y * 0.3f;
        }
        else if (transform.position.y > WORLD_HEIGHT)
        {
            transform.position.y = WORLD_HEIGHT;
            velocity.velocity.y = -velocity.velocity.y * 0.3f;
        }

        velocity.speed = clamp(velocity.velocity.length(), 0.0f, velocity.maxSpeed);

        if (reg.hasComponent<HealthComponent>(entityId))
        {
            HealthComponent& health = reg.getComponent<HealthComponent>(entityId);
            health.iFrameTimer = std::max(0.0f, health.iFrameTimer - dt);
        }
    }

    worldTree.clear();
    for (const std::uint32_t entityId : reg.view<TransformComponent, ColliderComponent>())
    {
        const TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
        const ColliderComponent& collider = reg.getComponent<ColliderComponent>(entityId);
        worldTree.insert(entityId, buildBounds(transform, collider));
    }

    std::unordered_set<std::uint64_t> processedPairs;
    for (const std::uint32_t entityId : reg.view<TransformComponent, ColliderComponent>())
    {
        const TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
        const ColliderComponent& collider = reg.getComponent<ColliderComponent>(entityId);
        const Rect queryBounds = buildBounds(transform, collider).expanded(4.0f);

        std::vector<std::uint32_t> candidates;
        worldTree.query(queryBounds, candidates);

        for (const std::uint32_t candidateId : candidates)
        {
            if (candidateId == entityId || !reg.isAlive(candidateId))
            {
                continue;
            }

            const std::uint64_t pairKey = makePairKey(entityId, candidateId);
            if (!processedPairs.insert(pairKey).second)
            {
                continue;
            }

            const Rect selfBounds = buildBounds(reg.getComponent<TransformComponent>(entityId), collider);
            const TransformComponent& otherTransform = reg.getComponent<TransformComponent>(candidateId);
            const ColliderComponent& otherCollider = reg.getComponent<ColliderComponent>(candidateId);
            const Rect otherBounds = buildBounds(otherTransform, otherCollider);

            if (!selfBounds.intersects(otherBounds))
            {
                continue;
            }

            collisions.push_back(CollisionEvent{entityId, candidateId});

            if (!(collider.solid && otherCollider.solid))
            {
                continue;
            }

            const float selfLeft = selfBounds.x;
            const float selfRight = selfBounds.x + selfBounds.w;
            const float selfTop = selfBounds.y;
            const float selfBottom = selfBounds.y + selfBounds.h;

            const float otherLeft = otherBounds.x;
            const float otherRight = otherBounds.x + otherBounds.w;
            const float otherTop = otherBounds.y;
            const float otherBottom = otherBounds.y + otherBounds.h;

            const float overlapX = std::min(selfRight, otherRight) - std::max(selfLeft, otherLeft);
            const float overlapY = std::min(selfBottom, otherBottom) - std::max(selfTop, otherTop);
            if (overlapX <= 0.0f || overlapY <= 0.0f)
            {
                continue;
            }

            TransformComponent& mutableSelfTransform = reg.getComponent<TransformComponent>(entityId);
            TransformComponent& mutableOtherTransform = reg.getComponent<TransformComponent>(candidateId);

            const Vector2 selfCenter = selfBounds.center();
            const Vector2 otherCenter = otherBounds.center();

            if (overlapX < overlapY)
            {
                const float correction = overlapX * 0.5f;
                if (selfCenter.x < otherCenter.x)
                {
                    mutableSelfTransform.position.x -= correction;
                    mutableOtherTransform.position.x += correction;
                }
                else
                {
                    mutableSelfTransform.position.x += correction;
                    mutableOtherTransform.position.x -= correction;
                }
            }
            else
            {
                const float correction = overlapY * 0.5f;
                if (selfCenter.y < otherCenter.y)
                {
                    mutableSelfTransform.position.y -= correction;
                    mutableOtherTransform.position.y += correction;
                }
                else
                {
                    mutableSelfTransform.position.y += correction;
                    mutableOtherTransform.position.y -= correction;
                }
            }
        }
    }
}

const std::vector<PhysicsSystem::CollisionEvent>& PhysicsSystem::getCollisions() const
{
    return collisions;
}

Quadtree& PhysicsSystem::getQuadtree()
{
    return worldTree;
}

const Quadtree& PhysicsSystem::getQuadtree() const
{
    return worldTree;
}

void PhysicsSystem::clearCollisions()
{
    collisions.clear();
}

Rect PhysicsSystem::buildBounds(const TransformComponent& transform, const ColliderComponent& collider)
{
    return Rect(
        transform.position.x - (collider.width * 0.5f),
        transform.position.y - (collider.height * 0.5f),
        collider.width,
        collider.height
    );
}
