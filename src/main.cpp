#include "AssetManager.h"
#include "Engine.h"
#include "GameScene.h"

#include <iostream>

class GameApp final : public Engine
{
public:
    GameApp()
        : assets(AssetManager::get()),
          scene()
    {
    }

protected:
    void onInit() override
    {
        assets.loadTexture("player", "assets/sprites/player.png", getRenderer());
        assets.loadTexture("bullet_player", "assets/sprites/bullet_player.png", getRenderer());
        assets.loadTexture("bullet_enemy", "assets/sprites/bullet_enemy.png", getRenderer());
        assets.loadTexture("enemy_grunt", "assets/sprites/enemy_grunt.png", getRenderer());
        assets.loadTexture("enemy_flanker", "assets/sprites/enemy_flanker.png", getRenderer());
        assets.loadTexture("enemy_tank", "assets/sprites/enemy_tank.png", getRenderer());
        assets.loadTexture("enemy_sniper", "assets/sprites/enemy_sniper.png", getRenderer());
        assets.loadTexture("boss", "assets/sprites/boss.png", getRenderer());
        assets.loadTexture("pickup_health", "assets/sprites/pickup_health.png", getRenderer());
        assets.loadTexture("wall_tile", "assets/sprites/wall_tile.png", getRenderer());

        assets.loadSound("shoot_player", "assets/audio/shoot_player.wav");
        assets.loadSound("shoot_enemy", "assets/audio/shoot_enemy.wav");
        assets.loadSound("explosion_sm", "assets/audio/explosion_small.wav");
        assets.loadSound("explosion_lg", "assets/audio/explosion_large.wav");
        assets.loadSound("player_hit", "assets/audio/player_hit.wav");
        assets.loadSound("pickup", "assets/audio/pickup.wav");
        assets.loadSound("boss_roar", "assets/audio/boss_roar.wav");

        assets.loadMusic("bgm", "assets/audio/bgm.ogg");
        assets.loadFont("hud", "assets/fonts/font.ttf", 22);
        assets.loadFont("title", "assets/fonts/font.ttf", 48);

        std::cout << "Assets loaded." << std::endl;
        scene.init(getRenderer(), assets);
    }

    void onEvent(const SDL_Event& event) override
    {
        scene.handleEvent(event);
    }

    void onUpdate(float dt) override
    {
        scene.update(dt);
    }

    void onRender(float alpha) override
    {
        scene.render(getRenderer(), assets, alpha);
    }

    void onShutdown() override
    {
        assets.cleanup();
    }

private:
    AssetManager& assets;
    GameScene scene;
};

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "PratStrike — Started" << std::endl;

    GameApp app;
    if (!app.init("PratStrike", 1280, 720))
    {
        std::cerr << "PratStrike — Initialization failed" << std::endl;
        return 1;
    }

    app.run();
    app.shutdown();

    std::cout << "PratStrike — Shutdown" << std::endl;
    return 0;
}
