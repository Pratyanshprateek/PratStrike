#include "RenderSystem.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "GameFeel.h"

namespace
{
    constexpr float SCREEN_WIDTH = 1280.0f;
    constexpr float SCREEN_HEIGHT = 720.0f;
    constexpr float WORLD_WIDTH = 2560.0f;
    constexpr float WORLD_HEIGHT = 1440.0f;
    constexpr float FIXED_RENDER_DT = 1.0f / 60.0f;

    void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color color)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        for (int y = -radius; y <= radius; ++y)
        {
            const int xSpan = static_cast<int>(std::sqrt(static_cast<float>((radius * radius) - (y * y))));
            SDL_RenderDrawLine(renderer, centerX - xSpan, centerY + y, centerX + xSpan, centerY + y);
        }
    }

    void drawRoundedBar(SDL_Renderer* renderer, int x, int y, int width, int height, SDL_Color color)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        const int radius = height / 2;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        if (width <= radius * 2)
        {
            drawFilledCircle(renderer, x + (width / 2), y + radius, width / 2, color);
            return;
        }

        const SDL_Rect centerRect{x + radius, y, width - (radius * 2), height};
        SDL_RenderFillRect(renderer, &centerRect);
        drawFilledCircle(renderer, x + radius, y + radius, radius, color);
        drawFilledCircle(renderer, x + width - radius, y + radius, radius, color);
    }

    void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color, bool alignRight = false)
    {
        if (font == nullptr || text.empty())
        {
            return;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (surface == nullptr)
        {
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture == nullptr)
        {
            SDL_FreeSurface(surface);
            return;
        }

        SDL_Rect dst{x, y, surface->w, surface->h};
        if (alignRight)
        {
            dst.x -= dst.w;
        }

        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
}

RenderSystem::RenderSystem()
    : cameraPos(),
      screenCenter(640.0f, 360.0f),
      playerEntityId(0),
      currentWave(1),
      bossWaveActive(false),
      particleEntities()
{
}

void RenderSystem::init(std::uint32_t playerId)
{
    playerEntityId = playerId;
    cameraPos = Vector2();
}

void RenderSystem::setWaveInfo(int waveNumber, bool bossWave)
{
    currentWave = waveNumber;
    bossWaveActive = bossWave;
}

void RenderSystem::update(Registry& reg, float dt)
{
    for (const std::uint32_t entityId : reg.view<SpriteComponent>())
    {
        SpriteComponent& sprite = reg.getComponent<SpriteComponent>(entityId);
        if (sprite.totalFrames > 1 && sprite.animSpeed > 0.0f)
        {
            sprite.animTimer += dt;
            while (sprite.animTimer >= sprite.animSpeed)
            {
                sprite.animTimer -= sprite.animSpeed;
                sprite.currentFrame = (sprite.currentFrame + 1) % sprite.totalFrames;
            }
        }

        if (reg.hasComponent<HitstopComponent>(entityId) && !isWorldHitstopEntity(entityId))
        {
            HitstopComponent& flashTimer = reg.getComponent<HitstopComponent>(entityId);
            flashTimer.timer = std::max(0.0f, flashTimer.timer - dt);
            if (flashTimer.timer <= 0.0f)
            {
                sprite.tintColor = SDL_Color{255, 255, 255, 255};
            }
        }
    }

    std::vector<std::uint32_t> particlesToDestroy;
    particleEntities.clear();

    for (const std::uint32_t entityId : reg.view<ParticleComponent>())
    {
        ParticleComponent& particle = reg.getComponent<ParticleComponent>(entityId);
        particle.lifetime -= dt;
        particle.position += particle.velocity * dt;

        if (particle.lifetime <= 0.0f)
        {
            particlesToDestroy.push_back(entityId);
            continue;
        }

        particleEntities.push_back(entityId);
    }

    std::sort(
        particleEntities.begin(),
        particleEntities.end(),
        [&reg](std::uint32_t a, std::uint32_t b)
        {
            return reg.getComponent<ParticleComponent>(a).position.y < reg.getComponent<ParticleComponent>(b).position.y;
        }
    );

    for (const std::uint32_t entityId : particlesToDestroy)
    {
        reg.destroyEntity(entityId);
    }

    for (const std::uint32_t entityId : reg.view<ScreenshakeComponent>())
    {
        ScreenshakeComponent& shake = reg.getComponent<ScreenshakeComponent>(entityId);
        shake.timer = std::max(0.0f, shake.timer - dt);

        if (shake.timer > 0.0f && shake.duration > 0.0f)
        {
            const float magnitude = shake.intensity * (shake.timer / shake.duration);
            const float angle = randomFloat(0.0f, 360.0f);
            const float radius = randomFloat(0.0f, magnitude);
            shake.offset = Vector2(0.0f, -radius).rotated(angle);
        }
        else
        {
            shake.offset = Vector2();
        }
    }
}

void RenderSystem::render(SDL_Renderer* renderer, Registry& reg, AssetManager& assets, float alpha)
{
    if (playerEntityId != 0U && reg.isAlive(playerEntityId) && reg.hasComponent<TransformComponent>(playerEntityId))
    {
        const Vector2 playerWorldPos = reg.getComponent<TransformComponent>(playerEntityId).position;
        const Vector2 targetCamera = Vector2(playerWorldPos.x - screenCenter.x, playerWorldPos.y - screenCenter.y);
        cameraPos = cameraPos.lerp(targetCamera, 8.0f * FIXED_RENDER_DT);
        cameraPos.x = clamp(cameraPos.x, 0.0f, WORLD_WIDTH - SCREEN_WIDTH);
        cameraPos.y = clamp(cameraPos.y, 0.0f, WORLD_HEIGHT - SCREEN_HEIGHT);
    }

    Vector2 accumulatedShake;
    for (const std::uint32_t entityId : reg.view<ScreenshakeComponent>())
    {
        const ScreenshakeComponent& shake = reg.getComponent<ScreenshakeComponent>(entityId);
        if (shake.timer > 0.0f)
        {
            accumulatedShake += shake.offset;
        }
    }

    const Vector2 baseCameraPos = cameraPos;
    cameraPos += accumulatedShake;

    std::vector<std::uint32_t> spriteEntities = reg.view<TransformComponent, SpriteComponent>();
    std::sort(
        spriteEntities.begin(),
        spriteEntities.end(),
        [&reg](std::uint32_t a, std::uint32_t b)
        {
            return reg.getComponent<TransformComponent>(a).position.y < reg.getComponent<TransformComponent>(b).position.y;
        }
    );

    for (const std::uint32_t entityId : spriteEntities)
    {
        renderSprite(renderer, entityId, reg, alpha);
    }

    for (const std::uint32_t entityId : spriteEntities)
    {
        renderHealthBar(renderer, entityId, reg);
    }

    renderParticles(renderer, reg, alpha);
    renderHUD(renderer, reg, assets.getFont("hud"), assets.getFont("title"));

    cameraPos = baseCameraPos;
}

Vector2 RenderSystem::getCameraPos() const
{
    return cameraPos;
}

Vector2 RenderSystem::worldToScreen(Vector2 worldPos) const
{
    return worldPos - cameraPos;
}

bool RenderSystem::isOnScreen(Vector2 worldPos, float margin) const
{
    return worldPos.x > cameraPos.x - margin &&
           worldPos.x < cameraPos.x + SCREEN_WIDTH + margin &&
           worldPos.y > cameraPos.y - margin &&
           worldPos.y < cameraPos.y + SCREEN_HEIGHT + margin;
}

void RenderSystem::renderSprite(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg, float alpha)
{
    SpriteComponent& sprite = reg.getComponent<SpriteComponent>(entityId);
    TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);

    if (sprite.texture == nullptr || sprite.frameW <= 0 || sprite.frameH <= 0 || sprite.totalFrames <= 0)
    {
        return;
    }

    const Vector2 renderPos = transform.prevPosition.lerp(transform.position, alpha);
    const float renderRot = lerpF(transform.prevRotation, transform.rotation, alpha);

    if (!isOnScreen(renderPos))
    {
        return;
    }

    if (reg.hasComponent<HealthComponent>(entityId))
    {
        const HealthComponent& health = reg.getComponent<HealthComponent>(entityId);
        if (health.iFrameTimer > 0.0f)
        {
            const std::uint64_t flickerFrame = SDL_GetTicks64() / 16U;
            if ((flickerFrame % 4U) < 2U)
            {
                return;
            }
        }
    }

    const Vector2 screenPos = worldToScreen(renderPos);
    SDL_SetTextureColorMod(sprite.texture, sprite.tintColor.r, sprite.tintColor.g, sprite.tintColor.b);

    const int safeFrame = (sprite.totalFrames > 0) ? (sprite.currentFrame % sprite.totalFrames) : 0;
    const SDL_Rect src{safeFrame * sprite.frameW, 0, sprite.frameW, sprite.frameH};
    const SDL_Rect dst{
        static_cast<int>(screenPos.x - (sprite.frameW * 0.5f)),
        static_cast<int>(screenPos.y - (sprite.frameH * 0.5f)),
        sprite.frameW,
        sprite.frameH
    };
    const SDL_Point center{sprite.frameW / 2, sprite.frameH / 2};

    SDL_RenderCopyEx(renderer, sprite.texture, &src, &dst, renderRot, &center, SDL_FLIP_NONE);
    renderIFrameFlicker(renderer, entityId, reg);
    SDL_SetTextureColorMod(sprite.texture, 255, 255, 255);
}

void RenderSystem::renderHealthBar(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg)
{
    if (entityId == playerEntityId || !reg.hasComponent<HealthComponent>(entityId) || !reg.hasComponent<TransformComponent>(entityId))
    {
        return;
    }

    const TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
    const HealthComponent& health = reg.getComponent<HealthComponent>(entityId);

    if (!isOnScreen(transform.position))
    {
        return;
    }

    const Vector2 screenPos = worldToScreen(transform.position);
    const int barWidth = 40;
    const int barHeight = 5;
    const int x = static_cast<int>(screenPos.x - (barWidth / 2));
    const int y = static_cast<int>(screenPos.y - 32.0f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 70, 12, 12, 220);
    const SDL_Rect background{x, y, barWidth, barHeight};
    SDL_RenderFillRect(renderer, &background);

    const float hpRatio = (health.maxHp > 0) ? clamp(static_cast<float>(health.hp) / static_cast<float>(health.maxHp), 0.0f, 1.0f) : 0.0f;
    const SDL_Color fillColor{
        static_cast<Uint8>(lerpF(220.0f, 32.0f, hpRatio)),
        static_cast<Uint8>(lerpF(40.0f, 220.0f, hpRatio)),
        32,
        255
    };
    SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    const SDL_Rect fill{x, y, static_cast<int>(static_cast<float>(barWidth) * hpRatio), barHeight};
    SDL_RenderFillRect(renderer, &fill);
}

void RenderSystem::renderParticles(SDL_Renderer* renderer, Registry& reg, float alpha)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const std::uint32_t entityId : particleEntities)
    {
        if (!reg.isAlive(entityId) || !reg.hasComponent<ParticleComponent>(entityId))
        {
            continue;
        }

        const ParticleComponent& particle = reg.getComponent<ParticleComponent>(entityId);
        const Vector2 interpolatedPos = particle.position + (particle.velocity * (alpha * FIXED_RENDER_DT));
        if (!isOnScreen(interpolatedPos))
        {
            continue;
        }

        const float lifeRatio = (particle.maxLifetime > 0.0f) ? clamp(particle.lifetime / particle.maxLifetime, 0.0f, 1.0f) : 0.0f;
        const Uint8 alphaValue = static_cast<Uint8>(lifeRatio * 255.0f);
        const Vector2 screenPos = worldToScreen(interpolatedPos);

        SDL_SetRenderDrawColor(renderer, particle.color.r, particle.color.g, particle.color.b, alphaValue);
        const int size = static_cast<int>(particle.size);
        const SDL_Rect rect{
            static_cast<int>(screenPos.x - (particle.size * 0.5f)),
            static_cast<int>(screenPos.y - (particle.size * 0.5f)),
            size,
            size
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void RenderSystem::renderHUD(SDL_Renderer* renderer, Registry& reg, TTF_Font* hudFont, TTF_Font* titleFont)
{
    if (playerEntityId == 0U || !reg.isAlive(playerEntityId) || !reg.hasComponent<PlayerStateComponent>(playerEntityId) || !reg.hasComponent<HealthComponent>(playerEntityId))
    {
        return;
    }

    const PlayerStateComponent& playerState = reg.getComponent<PlayerStateComponent>(playerEntityId);
    const HealthComponent& playerHealth = reg.getComponent<HealthComponent>(playerEntityId);
    const SDL_Color white{255, 255, 255, 255};

    drawText(renderer, hudFont, "SCORE: " + std::to_string(playerState.score), 24, 20, white);
    drawText(renderer, hudFont, "KILLS: " + std::to_string(playerState.kills), 24, 48, white);

    const std::string waveLabel = bossWaveActive ? ("BOSS WAVE " + std::to_string(currentWave)) : ("WAVE " + std::to_string(currentWave));
    drawText(renderer, titleFont, waveLabel, static_cast<int>(SCREEN_WIDTH) - 24, 18, white, true);

    const int barWidth = 200;
    const int barHeight = 16;
    const int barX = static_cast<int>((SCREEN_WIDTH * 0.5f) - (barWidth * 0.5f));
    const int barY = static_cast<int>(SCREEN_HEIGHT) - 42;

    const float hpRatio = (playerHealth.maxHp > 0) ? clamp(static_cast<float>(playerHealth.hp) / static_cast<float>(playerHealth.maxHp), 0.0f, 1.0f) : 0.0f;
    SDL_Color hpColor{220, 48, 48, 255};
    if (hpRatio > 0.6f)
    {
        hpColor = SDL_Color{48, 210, 90, 255};
    }
    else if (hpRatio > 0.3f)
    {
        hpColor = SDL_Color{235, 190, 40, 255};
    }

    drawText(renderer, hudFont, "HP", barX - 34, barY - 3, white);
    drawRoundedBar(renderer, barX, barY, barWidth, barHeight, SDL_Color{40, 40, 48, 220});
    drawRoundedBar(renderer, barX, barY, static_cast<int>(static_cast<float>(barWidth) * hpRatio), barHeight, hpColor);

    const bool dashReady = playerState.dashCooldown <= 0.0f;
    const SDL_Color dashColor = dashReady ? SDL_Color{40, 235, 255, 255} : SDL_Color{96, 96, 104, 255};
    drawFilledCircle(renderer, static_cast<int>(SCREEN_WIDTH) - 36, static_cast<int>(SCREEN_HEIGHT) - 34, 12, dashColor);
}

void RenderSystem::renderIFrameFlicker(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg)
{
    if (!reg.hasComponent<HealthComponent>(entityId) || !reg.hasComponent<TransformComponent>(entityId) || !reg.hasComponent<SpriteComponent>(entityId))
    {
        return;
    }

    const HealthComponent& health = reg.getComponent<HealthComponent>(entityId);
    if (health.iFrameTimer <= 0.0f)
    {
        return;
    }

    const TransformComponent& transform = reg.getComponent<TransformComponent>(entityId);
    const SpriteComponent& sprite = reg.getComponent<SpriteComponent>(entityId);
    const Vector2 screenPos = worldToScreen(transform.position);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
    const SDL_Rect outline{
        static_cast<int>(screenPos.x - (sprite.frameW * 0.5f)) - 1,
        static_cast<int>(screenPos.y - (sprite.frameH * 0.5f)) - 1,
        sprite.frameW + 2,
        sprite.frameH + 2
    };
    SDL_RenderDrawRect(renderer, &outline);
}
