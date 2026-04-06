#pragma once

#include <cstdint>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#include "AssetManager.h"
#include "Components.h"
#include "ECS.h"

class RenderSystem
{
public:
    RenderSystem();

    void init(std::uint32_t playerEntityId);
    void setWaveInfo(int waveNumber, bool bossWave);
    void update(Registry& reg, float dt);
    void render(SDL_Renderer* renderer, Registry& reg, AssetManager& assets, float alpha);
    Vector2 getCameraPos() const;

private:
    Vector2 worldToScreen(Vector2 worldPos) const;
    bool isOnScreen(Vector2 worldPos, float margin = 64.0f) const;

    void renderSprite(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg, float alpha);
    void renderHealthBar(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg);
    void renderParticles(SDL_Renderer* renderer, Registry& reg, float alpha);
    void renderHUD(SDL_Renderer* renderer, Registry& reg, TTF_Font* hudFont, TTF_Font* titleFont);
    void renderIFrameFlicker(SDL_Renderer* renderer, std::uint32_t entityId, Registry& reg);

    Vector2 cameraPos;
    Vector2 screenCenter;
    std::uint32_t playerEntityId;
    int currentWave;
    bool bossWaveActive;
    std::vector<std::uint32_t> particleEntities;
};
