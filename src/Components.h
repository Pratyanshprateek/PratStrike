#pragma once

#include <cstdint>

#include <SDL.h>

#include "Math.h"

struct TransformComponent
{
    Vector2 position;
    Vector2 prevPosition;
    float rotation;
    float prevRotation;
};

struct VelocityComponent
{
    Vector2 velocity;
    float speed;
    float maxSpeed;
    float acceleration;
    float friction;
    float turnSpeed;
};

struct ColliderComponent
{
    float width;
    float height;
    bool solid;
    bool isTrigger;
    std::uint8_t layer;
};

struct HealthComponent
{
    int hp;
    int maxHp;
    float iFrameTimer;
    float iFrameDuration;
    bool dead;
};

struct SpriteComponent
{
    SDL_Texture* texture;
    int frameW;
    int frameH;
    int totalFrames;
    int currentFrame;
    float animTimer;
    float animSpeed;
    SDL_Color tintColor;
};

struct TagComponent
{
    enum class Tag
    {
        Player,
        Enemy,
        EnemyBullet,
        PlayerBullet,
        Wall,
        Pickup,
        Boss
    };

    Tag tag;
};

struct PlayerStateComponent
{
    float fireCooldown;
    float fireRate;
    int score;
    int kills;
    bool isDashing;
    float dashTimer;
    float dashDuration;
    float dashCooldown;
    float dashCooldownMax;
    Vector2 dashDirection;
    bool wantsToFire;
};

struct BulletComponent
{
    Vector2 spawnPos;
    float maxRange;
    float damage;
    bool fromPlayer;
};

struct EnemyComponent
{
    enum class EnemyType
    {
        Grunt,
        Flanker,
        Tank,
        Sniper
    };

    enum class AIState
    {
        Patrol,
        Chase,
        Attack,
        Fleeing,
        Dead
    };

    EnemyType type;
    float detectionRadius;
    float attackRadius;
    float fireCooldown;
    float fireRate;
    AIState state;
    Vector2 patrolTarget;
    float patrolTimer;
    int pointValue;
};

struct BossComponent
{
    enum class BossAttack
    {
        Spread,
        Spiral,
        Laser,
        Charge
    };

    int phase;
    float phaseTimer;
    float attackTimer;
    float attackCooldown;
    BossAttack currentAttack;
    bool isCharging;
    Vector2 chargeDirection;
    float chargeSpeed;
    float chargeTimer;
};

struct HitstopComponent
{
    float timer;
    float duration;
};

struct ScreenshakeComponent
{
    float intensity;
    float timer;
    float duration;
    Vector2 offset;
};

struct ParticleComponent
{
    Vector2 position;
    Vector2 velocity;
    SDL_Color color;
    float lifetime;
    float maxLifetime;
    float size;
};
