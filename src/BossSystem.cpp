#include "BossSystem.h"

#include "GameFeel.h"

BossSystem::BossSystem()
    : active(false),
      bossEntityId(0),
      spiralAngle(0.0f),
      alternateCharge(false)
{
}

void BossSystem::spawnBoss(Registry& reg, AssetManager& assets, Vector2 worldCenter)
{
    bossEntityId = reg.createEntity();
    reg.addComponent(bossEntityId, TransformComponent{worldCenter, worldCenter, 0.0f, 0.0f});
    reg.addComponent(bossEntityId, VelocityComponent{Vector2(), 0.0f, 80.0f, 250.0f, 0.0f, 180.0f});
    reg.addComponent(bossEntityId, ColliderComponent{80.0f, 80.0f, true, false, 2});
    reg.addComponent(bossEntityId, HealthComponent{600, 600, 0.0f, 0.25f, false});
    reg.addComponent(bossEntityId, BossComponent{
        1,
        0.0f,
        0.5f,
        2.0f,
        BossComponent::BossAttack::Spread,
        false,
        Vector2(),
        400.0f,
        0.0f
    });
    reg.addComponent(bossEntityId, SpriteComponent{
        assets.getTexture("boss"),
        96,
        84,
        1,
        0,
        0.0f,
        1.0f,
        SDL_Color{255, 255, 255, 255}
    });
    reg.addComponent(bossEntityId, TagComponent{TagComponent::Tag::Boss});

    Mix_Chunk* roar = assets.getSound("boss_roar");
    if (roar != nullptr)
    {
        Mix_PlayChannel(-1, roar, 0);
    }

    active = true;
    spiralAngle = 0.0f;
    alternateCharge = false;
}

void BossSystem::update(Registry& reg,
                        std::uint32_t playerEntityId,
                        BulletSystem& bullets,
                        AssetManager& assets,
                        float dt)
{
    if (!active ||
        !reg.isAlive(bossEntityId) ||
        !reg.isAlive(playerEntityId) ||
        isWorldHitstopActive(reg))
    {
        return;
    }

    BossComponent& boss = reg.getComponent<BossComponent>(bossEntityId);
    HealthComponent& health = reg.getComponent<HealthComponent>(bossEntityId);
    TransformComponent& transform = reg.getComponent<TransformComponent>(bossEntityId);
    VelocityComponent& velocity = reg.getComponent<VelocityComponent>(bossEntityId);
    const Vector2 playerPos = reg.getComponent<TransformComponent>(playerEntityId).position;

    if (health.dead || health.hp <= 0)
    {
        return;
    }

    const int previousPhase = boss.phase;
    if (health.hp > 400)
    {
        boss.phase = 1;
    }
    else if (health.hp > 200)
    {
        boss.phase = 2;
    }
    else
    {
        boss.phase = 3;
    }

    if (boss.phase != previousPhase)
    {
        triggerScreenshake(reg, playerEntityId, 12.0f, 0.6f);
        triggerHitstop(reg, 0.10f);
        spawnDeathParticles(reg, transform.position, SDL_Color{255, 170, 60, 255}, 20, 260.0f, 0.7f);
        playSpatialSound(assets, "explosion_lg", transform.position, playerPos);

        if (boss.phase == 2)
        {
            velocity.maxSpeed = 130.0f;
        }
        else if (boss.phase == 3)
        {
            velocity.maxSpeed = 180.0f;
            boss.attackCooldown *= 0.5f;
        }
    }

    boss.phaseTimer += dt;
    boss.attackTimer -= dt;

    if (boss.isCharging)
    {
        velocity.velocity = boss.chargeDirection * boss.chargeSpeed;
        velocity.speed = boss.chargeSpeed;
        transform.rotation = boss.chargeDirection.angle();
        boss.chargeTimer -= dt;
        if (boss.chargeTimer <= 0.0f)
        {
            boss.isCharging = false;
            velocity.speed = 0.0f;
        }
    }
    else
    {
        const Vector2 toPlayer = (playerPos - transform.position).normalized();
        transform.rotation = toPlayer.angle();
        velocity.speed = velocity.maxSpeed * 0.35f;
        velocity.velocity = toPlayer * velocity.speed;
    }

    if (boss.attackTimer > 0.0f)
    {
        return;
    }

    if (boss.phase == 1)
    {
        const float centerAngle = (playerPos - transform.position).angle();
        for (int i = -2; i <= 2; ++i)
        {
            bullets.spawnBullet(reg,
                                transform.position,
                                centerAngle + (static_cast<float>(i) * 15.0f),
                                280.0f,
                                18.0f,
                                false,
                                assets.getTexture("bullet_enemy"),
                                1100.0f);
        }
        boss.currentAttack = BossComponent::BossAttack::Spread;
        boss.attackCooldown = 2.0f;
    }
    else if (boss.phase == 2)
    {
        for (int i = 0; i < 8; ++i)
        {
            const float angle = spiralAngle + (static_cast<float>(i) * 45.0f);
            bullets.spawnBullet(reg,
                                transform.position,
                                angle,
                                300.0f,
                                20.0f,
                                false,
                                assets.getTexture("bullet_enemy"),
                                1100.0f);
        }
        spiralAngle += 22.5f;
        boss.currentAttack = BossComponent::BossAttack::Spiral;
        boss.attackCooldown = 1.2f;
    }
    else
    {
        if (alternateCharge)
        {
            boss.currentAttack = BossComponent::BossAttack::Charge;
            boss.isCharging = true;
            boss.chargeDirection = (playerPos - transform.position).normalized();
            boss.chargeSpeed = 400.0f;
            boss.chargeTimer = 0.4f;
        }
        else
        {
            boss.currentAttack = BossComponent::BossAttack::Spiral;
            for (int i = 0; i < 12; ++i)
            {
                const float angle = spiralAngle + (static_cast<float>(i) * 30.0f);
                bullets.spawnBullet(reg,
                                    transform.position,
                                    angle,
                                    340.0f,
                                    24.0f,
                                    false,
                                    assets.getTexture("bullet_enemy"),
                                    1200.0f);
            }
            spiralAngle += 15.0f;
        }

        alternateCharge = !alternateCharge;
        boss.attackCooldown = 0.9f;
    }

    playSpatialSound(assets, "shoot_enemy", transform.position, playerPos);
    boss.attackTimer = boss.attackCooldown;
}

bool BossSystem::isDefeated(Registry& reg)
{
    return active && (!reg.isAlive(bossEntityId) || reg.getComponent<HealthComponent>(bossEntityId).dead);
}

void BossSystem::reset()
{
    active = false;
    bossEntityId = 0;
    spiralAngle = 0.0f;
    alternateCharge = false;
}
