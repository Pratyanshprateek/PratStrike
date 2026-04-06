#include "AISystem.h"

#include <algorithm>

#include "GameFeel.h"

namespace
{
    constexpr int DIFFICULTY_EASY = 0;
    constexpr int DIFFICULTY_MEDIUM = 1;
    constexpr int DIFFICULTY_HARD = 2;

    void configureEnemyType(EnemyComponent& enemy, VelocityComponent& velocity, ColliderComponent* collider)
    {
        switch (enemy.type)
        {
        case EnemyComponent::EnemyType::Grunt:
            velocity.maxSpeed = 120.0f;
            enemy.detectionRadius = 360.0f;
            enemy.attackRadius = 300.0f;
            enemy.fireRate = 0.85f;
            enemy.pointValue = 100;
            break;
        case EnemyComponent::EnemyType::Flanker:
            velocity.maxSpeed = 200.0f;
            enemy.detectionRadius = 380.0f;
            enemy.attackRadius = 300.0f;
            enemy.fireRate = 0.8f;
            enemy.pointValue = 150;
            break;
        case EnemyComponent::EnemyType::Tank:
            velocity.maxSpeed = 70.0f;
            enemy.detectionRadius = 320.0f;
            enemy.attackRadius = 250.0f;
            enemy.fireRate = 2.0f;
            enemy.pointValue = 300;
            if (collider != nullptr)
            {
                collider->solid = true;
            }
            break;
        case EnemyComponent::EnemyType::Sniper:
            velocity.maxSpeed = 90.0f;
            enemy.detectionRadius = 620.0f;
            enemy.attackRadius = 520.0f;
            enemy.fireRate = 3.5f;
            enemy.pointValue = 250;
            break;
        }
    }

    float rangeMultiplierForDifficulty(int difficultyIndex)
    {
        switch (difficultyIndex)
        {
        case DIFFICULTY_EASY:
            return 1.00f;
        case DIFFICULTY_HARD:
            return 1.18f;
        case DIFFICULTY_MEDIUM:
        default:
            return 1.10f;
        }
    }

    float leadFactorForDifficulty(int difficultyIndex)
    {
        switch (difficultyIndex)
        {
        case DIFFICULTY_EASY:
            return 0.10f;
        case DIFFICULTY_HARD:
            return 0.45f;
        case DIFFICULTY_MEDIUM:
        default:
            return 0.25f;
        }
    }

    float aimErrorForDifficulty(int difficultyIndex, EnemyComponent::EnemyType type)
    {
        float errorDegrees = 12.0f;
        switch (difficultyIndex)
        {
        case DIFFICULTY_EASY:
            errorDegrees = 18.0f;
            break;
        case DIFFICULTY_HARD:
            errorDegrees = 7.0f;
            break;
        case DIFFICULTY_MEDIUM:
        default:
            errorDegrees = 12.0f;
            break;
        }

        if (type == EnemyComponent::EnemyType::Sniper)
        {
            errorDegrees *= 0.65f;
        }
        else if (type == EnemyComponent::EnemyType::Tank)
        {
            errorDegrees *= 1.20f;
        }

        return errorDegrees;
    }

    float pressureRadiusForEnemy(const EnemyComponent& enemy)
    {
        switch (enemy.type)
        {
        case EnemyComponent::EnemyType::Grunt: return 380.0f;
        case EnemyComponent::EnemyType::Flanker: return 360.0f;
        case EnemyComponent::EnemyType::Tank: return 320.0f;
        case EnemyComponent::EnemyType::Sniper: return 640.0f;
        }
        return 360.0f;
    }

    float chaseFireCadenceScale(int difficultyIndex)
    {
        switch (difficultyIndex)
        {
        case DIFFICULTY_EASY: return 1.75f;
        case DIFFICULTY_HARD: return 1.25f;
        case DIFFICULTY_MEDIUM:
        default:
            return 1.45f;
        }
    }

    float damageForEnemy(const EnemyComponent& enemy)
    {
        switch (enemy.type)
        {
        case EnemyComponent::EnemyType::Grunt: return 10.0f;
        case EnemyComponent::EnemyType::Flanker: return 8.0f;
        case EnemyComponent::EnemyType::Tank: return 25.0f;
        case EnemyComponent::EnemyType::Sniper: return 40.0f;
        }
        return 10.0f;
    }

    float bulletSpeedForEnemy(const EnemyComponent& enemy)
    {
        switch (enemy.type)
        {
        case EnemyComponent::EnemyType::Sniper: return 520.0f;
        case EnemyComponent::EnemyType::Tank: return 260.0f;
        case EnemyComponent::EnemyType::Flanker: return 340.0f;
        case EnemyComponent::EnemyType::Grunt: return 300.0f;
        }
        return 300.0f;
    }
}

void AISystem::update(Registry& reg,
                      Quadtree& worldTree,
                      std::uint32_t playerEntityId,
                      int difficultyIndex,
                      BulletSystem& bullets,
                      AssetManager& assets,
                      float dt)
{
    if (!reg.isAlive(playerEntityId) ||
        !reg.hasComponent<TransformComponent>(playerEntityId) ||
        isWorldHitstopActive(reg))
    {
        return;
    }

    const Vector2 playerPos = reg.getComponent<TransformComponent>(playerEntityId).position;
    const Vector2 playerVelocity = reg.hasComponent<VelocityComponent>(playerEntityId)
                                       ? reg.getComponent<VelocityComponent>(playerEntityId).velocity
                                       : Vector2();

    for (const std::uint32_t entityId : reg.view<EnemyComponent, TransformComponent, VelocityComponent>())
    {
        EnemyComponent& enemy = reg.getComponent<EnemyComponent>(entityId);
        TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
        VelocityComponent& velocity = reg.getComponent<VelocityComponent>(entityId);
        ColliderComponent* collider = reg.hasComponent<ColliderComponent>(entityId) ? &reg.getComponent<ColliderComponent>(entityId) : nullptr;
        configureEnemyType(enemy, velocity, collider);
        enemy.detectionRadius *= rangeMultiplierForDifficulty(difficultyIndex);
        enemy.attackRadius *= rangeMultiplierForDifficulty(difficultyIndex);

        if (reg.hasComponent<HealthComponent>(entityId))
        {
            const HealthComponent& health = reg.getComponent<HealthComponent>(entityId);
            if (health.dead || health.hp <= 0)
            {
                enemy.state = EnemyComponent::AIState::Dead;
                velocity.speed = 0.0f;
                continue;
            }
        }

        const float distToPlayer = transform.position.distance(playerPos);
        if (enemy.state == EnemyComponent::AIState::Patrol && distToPlayer < enemy.detectionRadius)
        {
            enemy.state = EnemyComponent::AIState::Chase;
        }
        else if (enemy.state == EnemyComponent::AIState::Chase && distToPlayer < enemy.attackRadius)
        {
            enemy.state = EnemyComponent::AIState::Attack;
        }
        else if (enemy.state == EnemyComponent::AIState::Attack && distToPlayer > enemy.attackRadius * 1.3f)
        {
            enemy.state = EnemyComponent::AIState::Chase;
        }
        else if (enemy.state == EnemyComponent::AIState::Chase && distToPlayer > enemy.detectionRadius * 1.5f)
        {
            enemy.state = EnemyComponent::AIState::Patrol;
        }

        Vector2 desiredDirection;
        float targetSpeed = 0.0f;

        if (enemy.state == EnemyComponent::AIState::Patrol)
        {
            enemy.patrolTimer -= dt;
            if (enemy.patrolTimer <= 0.0f || transform.position.distance(enemy.patrolTarget) < 20.0f)
            {
                enemy.patrolTarget = Vector2(randomFloat(200.0f, 2360.0f), randomFloat(200.0f, 1240.0f));
                enemy.patrolTimer = randomFloat(1.5f, 3.5f);
            }

            desiredDirection = (enemy.patrolTarget - transform.position).normalized();
            targetSpeed = velocity.maxSpeed * 0.4f;
        }
        else if (enemy.state == EnemyComponent::AIState::Chase)
        {
            Vector2 chaseTarget = playerPos;
            const Vector2 toPlayer = (playerPos - transform.position).normalized();
            if (enemy.type == EnemyComponent::EnemyType::Flanker)
            {
                const Vector2 perpendicular(-toPlayer.y, toPlayer.x);
                chaseTarget += perpendicular * 80.0f;
            }

            Vector2 desired = (chaseTarget - transform.position).normalized() * velocity.maxSpeed;
            Vector2 separation;
            std::vector<std::uint32_t> neighbors;
            worldTree.queryCircle(transform.position, 80.0f, neighbors);

            for (const std::uint32_t otherId : neighbors)
            {
                if (otherId == entityId ||
                    !reg.isAlive(otherId) ||
                    !reg.hasComponent<EnemyComponent>(otherId) ||
                    !reg.hasComponent<TransformComponent>(otherId))
                {
                    continue;
                }

                const Vector2 offset = transform.position - reg.getComponent<TransformComponent>(otherId).position;
                const float distance = std::max(1.0f, offset.length());
                separation += offset.normalized() * (120.0f / distance);
            }

            const Vector2 finalVelocity = desired + separation;
            if (finalVelocity.lengthSq() > 0.0f)
            {
                desiredDirection = finalVelocity.normalized();
                targetSpeed = velocity.maxSpeed;
            }

            enemy.fireCooldown -= dt;
            const float pressureRadius = pressureRadiusForEnemy(enemy) * rangeMultiplierForDifficulty(difficultyIndex);
            if (distToPlayer <= pressureRadius && enemy.fireCooldown <= 0.0f)
            {
                const float bulletSpeed = bulletSpeedForEnemy(enemy);
                Vector2 aimTarget = playerPos;
                if (playerVelocity.lengthSq() > 0.0f)
                {
                    const float travelTime = distToPlayer / std::max(1.0f, bulletSpeed);
                    aimTarget = playerPos + (playerVelocity * travelTime * leadFactorForDifficulty(difficultyIndex) * 0.35f);
                }

                const Vector2 aimDirection = (aimTarget - transform.position).normalized();
                if (aimDirection.lengthSq() > 0.0f)
                {
                    const float angleError = aimErrorForDifficulty(difficultyIndex, enemy.type) * 1.4f;
                    bullets.spawnBullet(reg,
                                        transform.position,
                                        aimDirection.angle() + randomFloat(-angleError, angleError),
                                        bulletSpeed,
                                        damageForEnemy(enemy),
                                        false,
                                        assets.getTexture("bullet_enemy"));
                    playSpatialSound(assets, "shoot_enemy", transform.position, playerPos);
                    enemy.fireCooldown = enemy.fireRate * chaseFireCadenceScale(difficultyIndex);
                }
            }
        }
        else if (enemy.state == EnemyComponent::AIState::Attack)
        {
            enemy.fireCooldown -= dt;

            if (enemy.type == EnemyComponent::EnemyType::Sniper)
            {
                desiredDirection = Vector2();
                targetSpeed = 0.0f;
            }
            else
            {
                desiredDirection = (playerPos - transform.position).normalized();
                targetSpeed = velocity.maxSpeed * 0.6f;
            }

            Vector2 aimTarget = playerPos;
            const float bulletSpeed = bulletSpeedForEnemy(enemy);
            const float leadFactor = leadFactorForDifficulty(difficultyIndex);
            if (enemy.type == EnemyComponent::EnemyType::Sniper)
            {
                const float travelTime = distToPlayer / std::max(1.0f, bulletSpeed);
                aimTarget = playerPos + (playerVelocity * travelTime * leadFactor);
            }
            else if (playerVelocity.lengthSq() > 0.0f)
            {
                const float travelTime = distToPlayer / std::max(1.0f, bulletSpeed);
                aimTarget = playerPos + (playerVelocity * travelTime * leadFactor * 0.35f);
            }

            const Vector2 aimDirection = (aimTarget - transform.position).normalized();
            if (aimDirection.lengthSq() > 0.0f)
            {
                transform.rotation = aimDirection.angle();
            }

            if (enemy.fireCooldown <= 0.0f)
            {
                enemy.fireCooldown = enemy.fireRate;
                const float angleError = aimErrorForDifficulty(difficultyIndex, enemy.type);
                bullets.spawnBullet(reg,
                                    transform.position,
                                    aimDirection.angle() + randomFloat(-angleError, angleError),
                                    bulletSpeed,
                                    damageForEnemy(enemy),
                                    false,
                                    assets.getTexture("bullet_enemy"));
                playSpatialSound(assets, "shoot_enemy", transform.position, playerPos);
            }
        }

        if (desiredDirection.lengthSq() > 0.0f)
        {
            transform.rotation = desiredDirection.angle();
            velocity.velocity = desiredDirection * targetSpeed;
        }
        else
        {
            velocity.velocity = Vector2();
        }
        velocity.speed = targetSpeed;
    }
}
