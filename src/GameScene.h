#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AISystem.h"
#include "AssetManager.h"
#include "BossSystem.h"
#include "BulletSystem.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "WaveSystem.h"

class GameScene
{
public:
    struct ScoreEntry
    {
        std::string name;
        int score;
        int wave;
        std::string difficulty;
    };

    GameScene();

    void init(SDL_Renderer* renderer, AssetManager& assets);
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render(SDL_Renderer* renderer, AssetManager& assets, float alpha);

private:
    enum class SceneState
    {
        Menu,
        Playing,
        Paused,
        GameOver
    };

    void spawnPlayer();
    void handleCollisions();
    void checkGameOver();
    void spawnPickup(Vector2 pos);
    void resetGame();
    void endRunToMenu();
    void requestQuitGame();
    void loadLeaderboard();
    void saveLeaderboard() const;
    void submitScore();
    void renderMenu(SDL_Renderer* renderer, AssetManager& assets);
    void renderPaused(SDL_Renderer* renderer, AssetManager& assets, float alpha);
    void renderGameOver(SDL_Renderer* renderer, AssetManager& assets);
    std::string getDifficultyLabel() const;

    Registry registry;
    PhysicsSystem physics;
    RenderSystem renderSystem;
    InputSystem input;
    BulletSystem bullets;
    AISystem ai;
    BossSystem boss;
    WaveSystem waves;

    std::uint32_t playerEntityId;
    std::uint32_t cameraEntityId;
    SceneState sceneState;
    bool gameOver;
    bool gameStarted;
    bool scoreSubmitted;
    float gameOverTimer;
    int finalScore;
    int highestScore;
    int selectedDifficultyIndex;
    std::string pendingPlayerName;
    std::string currentPlayerName;
    std::vector<ScoreEntry> leaderboard;

    SDL_Renderer* rendererRef;
    AssetManager* assetsRef;
};
