#pragma once

#include <string>

#include <SDL_mixer.h>

#include "AssetManager.h"
#include "Components.h"
#include "ECS.h"
#include "Math.h"

inline std::uint32_t& worldHitstopEntityIdStorage()
{
    static std::uint32_t worldEntityId = 0;
    return worldEntityId;
}

inline bool isWorldHitstopEntity(std::uint32_t entityId)
{
    return entityId != 0U && entityId == worldHitstopEntityIdStorage();
}

inline bool isWorldHitstopActive(Registry& reg)
{
    const std::uint32_t worldEntityId = worldHitstopEntityIdStorage();
    return worldEntityId != 0U &&
           reg.isAlive(worldEntityId) &&
           reg.hasComponent<HitstopComponent>(worldEntityId) &&
           reg.getComponent<HitstopComponent>(worldEntityId).timer > 0.0f;
}

inline void triggerScreenshake(Registry& reg, std::uint32_t cameraEntityId, float intensity, float duration)
{
    if (!reg.isAlive(cameraEntityId))
    {
        return;
    }

    const ScreenshakeComponent shake{intensity, duration, duration, Vector2()};
    if (reg.hasComponent<ScreenshakeComponent>(cameraEntityId))
    {
        reg.getComponent<ScreenshakeComponent>(cameraEntityId) = shake;
    }
    else
    {
        reg.addComponent(cameraEntityId, shake);
    }
}

inline void triggerHitstop(Registry& reg, float duration)
{
    std::uint32_t& worldEntityId = worldHitstopEntityIdStorage();
    if (worldEntityId == 0U || !reg.isAlive(worldEntityId))
    {
        worldEntityId = reg.createEntity();
    }

    const HitstopComponent hitstop{duration, duration};
    if (reg.hasComponent<HitstopComponent>(worldEntityId))
    {
        reg.getComponent<HitstopComponent>(worldEntityId) = hitstop;
    }
    else
    {
        reg.addComponent(worldEntityId, hitstop);
    }
}

inline void spawnDeathParticles(Registry& reg, Vector2 worldPos, SDL_Color color, int count, float speed, float lifetime)
{
    for (int i = 0; i < count; ++i)
    {
        const std::uint32_t entityId = reg.createEntity();
        const Vector2 offset(randomFloat(-10.0f, 10.0f), randomFloat(-10.0f, 10.0f));
        const Vector2 velocity = Vector2(0.0f, -1.0f).rotated(randomFloat(0.0f, 360.0f)) * randomFloat(speed * 0.5f, speed);
        const float particleLifetime = randomFloat(lifetime * 0.5f, lifetime);

        reg.addComponent(entityId, ParticleComponent{
            worldPos + offset,
            velocity,
            SDL_Color{color.r, color.g, color.b, 255},
            particleLifetime,
            particleLifetime,
            randomFloat(2.0f, 5.0f)
        });
        reg.addComponent(entityId, TagComponent{TagComponent::Tag::Pickup});
    }
}

inline void triggerHitFlash(Registry& reg, std::uint32_t entityId, SDL_Color flashColor, float duration)
{
    if (!reg.isAlive(entityId) || !reg.hasComponent<SpriteComponent>(entityId))
    {
        return;
    }

    SpriteComponent& sprite = reg.getComponent<SpriteComponent>(entityId);
    sprite.tintColor = flashColor;

    const HitstopComponent timer{duration, duration};
    if (reg.hasComponent<HitstopComponent>(entityId))
    {
        reg.getComponent<HitstopComponent>(entityId) = timer;
    }
    else
    {
        reg.addComponent(entityId, timer);
    }
}

inline void playSpatialSound(AssetManager& assets,
                             const std::string& key,
                             Vector2 soundWorldPos,
                             Vector2 playerWorldPos,
                             int channel = -1,
                             float maxHearingRange = 800.0f)
{
    Mix_Chunk* chunk = assets.getSound(key);
    if (chunk == nullptr)
    {
        return;
    }

    const float dist = soundWorldPos.distance(playerWorldPos);
    if (dist > maxHearingRange)
    {
        return;
    }

    const int volume = static_cast<int>(clamp(1.0f - (dist / maxHearingRange), 0.0f, 1.0f) * 128.0f);
    Mix_VolumeChunk(chunk, volume);

    const int actualChannel = Mix_PlayChannel(channel, chunk, 0);
    if (actualChannel < 0)
    {
        return;
    }

    const float pan = clamp((soundWorldPos.x - playerWorldPos.x) / maxHearingRange, -1.0f, 1.0f);
    const Uint8 leftVol = static_cast<Uint8>(clamp(1.0f - pan, 0.0f, 1.0f) * 255.0f);
    const Uint8 rightVol = static_cast<Uint8>(clamp(1.0f + pan, 0.0f, 1.0f) * 255.0f);
    Mix_SetPanning(actualChannel, leftVol, rightVol);
}
