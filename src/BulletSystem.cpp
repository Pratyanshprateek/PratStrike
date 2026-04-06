#include "BulletSystem.h"

#include <unordered_set>

#include "AssetManager.h"

std::uint32_t BulletSystem::spawnBullet(Registry& reg,
                                        Vector2 worldPos,
                                        float angleDegrees,
                                        float speed,
                                        float damage,
                                        bool fromPlayer,
                                        SDL_Texture* texture,
                                        float maxRange)
{
    const std::uint32_t entityId = reg.createEntity();
    const float colliderWidth = fromPlayer ? 6.0f : 12.0f;
    const float colliderHeight = fromPlayer ? 12.0f : 20.0f;
    reg.addComponent(entityId, TransformComponent{worldPos, worldPos, angleDegrees, angleDegrees});
    reg.addComponent(entityId, VelocityComponent{
        Vector2(0.0f, -1.0f).rotated(angleDegrees) * speed,
        speed,
        speed,
        0.0f,
        0.0f,
        0.0f
    });
    reg.addComponent(entityId, ColliderComponent{
        colliderWidth,
        colliderHeight,
        false,
        true,
        static_cast<std::uint8_t>(fromPlayer ? 4 : 8)
    });
    reg.addComponent(entityId, BulletComponent{worldPos, maxRange, damage, fromPlayer});
    reg.addComponent(entityId, SpriteComponent{
        texture,
        6,
        12,
        1,
        0,
        0.0f,
        1.0f,
        SDL_Color{255, 255, 255, 255}
    });
    reg.addComponent(entityId, TagComponent{fromPlayer ? TagComponent::Tag::PlayerBullet : TagComponent::Tag::EnemyBullet});
    return entityId;
}

void BulletSystem::update(Registry& reg, const std::vector<PhysicsSystem::CollisionEvent>& collisions, float dt)
{
    (void)dt;

    std::unordered_set<std::uint32_t> bulletsToDestroy;
    std::uint32_t playerEntityId = 0;

    for (const std::uint32_t entityId : reg.view<TagComponent>())
    {
        if (reg.getComponent<TagComponent>(entityId).tag == TagComponent::Tag::Player)
        {
            playerEntityId = entityId;
            break;
        }
    }

    for (const std::uint32_t bulletId : reg.view<BulletComponent, TransformComponent>())
    {
        const BulletComponent& bullet = reg.getComponent<BulletComponent>(bulletId);
        const TransformComponent& transform = reg.getComponent<TransformComponent>(bulletId);
        if (transform.position.distance(bullet.spawnPos) > bullet.maxRange)
        {
            bulletsToDestroy.insert(bulletId);
        }
    }

    for (const PhysicsSystem::CollisionEvent& event : collisions)
    {
        std::uint32_t bulletId = 0;
        std::uint32_t targetId = 0;

        if (reg.isAlive(event.a) && reg.hasComponent<BulletComponent>(event.a))
        {
            bulletId = event.a;
            targetId = event.b;
        }
        else if (reg.isAlive(event.b) && reg.hasComponent<BulletComponent>(event.b))
        {
            bulletId = event.b;
            targetId = event.a;
        }
        else
        {
            continue;
        }

        if (!reg.isAlive(bulletId) || !reg.isAlive(targetId) || bulletsToDestroy.find(bulletId) != bulletsToDestroy.end())
        {
            continue;
        }

        const BulletComponent& bullet = reg.getComponent<BulletComponent>(bulletId);
        if (!reg.hasComponent<TagComponent>(targetId))
        {
            bulletsToDestroy.insert(bulletId);
            continue;
        }

        const TagComponent::Tag targetTag = reg.getComponent<TagComponent>(targetId).tag;
        const bool hitsEnemy = bullet.fromPlayer && (targetTag == TagComponent::Tag::Enemy || targetTag == TagComponent::Tag::Boss);
        const bool hitsPlayer = !bullet.fromPlayer && targetTag == TagComponent::Tag::Player;
        const bool hitsWall = targetTag == TagComponent::Tag::Wall;
        if (!hitsEnemy && !hitsPlayer && !hitsWall)
        {
            continue;
        }

        bulletsToDestroy.insert(bulletId);

        if (!reg.hasComponent<HealthComponent>(targetId) || hitsWall)
        {
            continue;
        }

        HealthComponent& health = reg.getComponent<HealthComponent>(targetId);
        if (hitsPlayer)
        {
            if (health.iFrameTimer > 0.0f)
            {
                continue;
            }

            health.hp -= static_cast<int>(bullet.damage);
            health.iFrameTimer = health.iFrameDuration;
            if (health.hp <= 0)
            {
                health.hp = 0;
                health.dead = true;
            }

            Mix_Chunk* playerHit = AssetManager::get().getSound("player_hit");
            if (playerHit != nullptr)
            {
                Mix_PlayChannel(-1, playerHit, 0);
            }
        }
        else if (hitsEnemy)
        {
            health.hp -= static_cast<int>(bullet.damage);
            if (health.hp <= 0)
            {
                health.hp = 0;
                health.dead = true;

                if (playerEntityId != 0U &&
                    reg.isAlive(playerEntityId) &&
                    reg.hasComponent<PlayerStateComponent>(playerEntityId) &&
                    reg.hasComponent<EnemyComponent>(targetId))
                {
                    PlayerStateComponent& playerState = reg.getComponent<PlayerStateComponent>(playerEntityId);
                    const EnemyComponent& enemy = reg.getComponent<EnemyComponent>(targetId);
                    playerState.kills += 1;
                    playerState.score += enemy.pointValue;
                }
            }
        }
    }

    for (const std::uint32_t bulletId : bulletsToDestroy)
    {
        if (reg.isAlive(bulletId))
        {
            reg.destroyEntity(bulletId);
        }
    }
}
