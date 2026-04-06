#include "GameScene.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <SDL_ttf.h>

#include "GameFeel.h"

namespace
{
    constexpr SDL_Rect MENU_NAME_BOX{360, 165, 560, 42};
    constexpr SDL_Rect MENU_DIFF_LEFT{420, 270, 48, 36};
    constexpr SDL_Rect MENU_DIFF_RIGHT{812, 270, 48, 36};
    constexpr SDL_Rect MENU_START_BUTTON{420, 340, 440, 44};
    constexpr SDL_Rect PAUSE_RESUME_BUTTON{430, 250, 420, 44};
    constexpr SDL_Rect PAUSE_MENU_BUTTON{430, 314, 420, 44};
    constexpr SDL_Rect PAUSE_EXIT_BUTTON{430, 378, 420, 44};

    bool pointInRect(int x, int y, const SDL_Rect& rect)
    {
        return x >= rect.x && x <= (rect.x + rect.w) &&
               y >= rect.y && y <= (rect.y + rect.h);
    }

    void drawCenteredText(SDL_Renderer* renderer,
                          TTF_Font* font,
                          const std::string& text,
                          int centerX,
                          int y,
                          SDL_Color color)
    {
        if (font == nullptr)
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

        SDL_Rect dst{centerX - (surface->w / 2), y, surface->w, surface->h};
        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
}

GameScene::GameScene()
    : registry(),
      physics(),
      renderSystem(),
      input(),
      bullets(),
      ai(),
      boss(),
      waves(),
      playerEntityId(0),
      cameraEntityId(0),
      sceneState(SceneState::Menu),
      gameOver(false),
      gameStarted(false),
      scoreSubmitted(false),
      gameOverTimer(0.0f),
      finalScore(0),
      highestScore(0),
      selectedDifficultyIndex(1),
      pendingPlayerName(),
      currentPlayerName(),
      leaderboard(),
      rendererRef(nullptr),
      assetsRef(nullptr)
{
}

void GameScene::init(SDL_Renderer* renderer, AssetManager& assets)
{
    rendererRef = renderer;
    assetsRef = &assets;
    waves.init();
    boss.reset();
    loadLeaderboard();
    pendingPlayerName.clear();
    currentPlayerName.clear();
    sceneState = SceneState::Menu;
    gameStarted = false;
    gameOver = false;
    scoreSubmitted = false;
    SDL_StartTextInput();
}

void GameScene::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_QUIT)
    {
        if (sceneState == SceneState::Playing || sceneState == SceneState::Paused)
        {
            finalScore = registry.isAlive(playerEntityId) && registry.hasComponent<PlayerStateComponent>(playerEntityId)
                             ? registry.getComponent<PlayerStateComponent>(playerEntityId).score
                             : finalScore;
            if (finalScore > 0)
            {
                submitScore();
            }
        }
        return;
    }

    if (sceneState == SceneState::Menu)
    {
        if (event.type == SDL_TEXTINPUT)
        {
            if (pendingPlayerName.size() < 12U)
            {
                pendingPlayerName += event.text.text;
            }
        }
        else if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_BACKSPACE && !pendingPlayerName.empty())
            {
                pendingPlayerName.pop_back();
            }
            else if (event.key.keysym.sym == SDLK_LEFT)
            {
                selectedDifficultyIndex = std::max(0, selectedDifficultyIndex - 1);
            }
            else if (event.key.keysym.sym == SDLK_RIGHT)
            {
                selectedDifficultyIndex = std::min(2, selectedDifficultyIndex + 1);
            }
            else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
            {
                currentPlayerName = pendingPlayerName.empty() ? "PLAYER" : pendingPlayerName;
                resetGame();
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            const int mouseX = event.button.x;
            const int mouseY = event.button.y;
            if (pointInRect(mouseX, mouseY, MENU_DIFF_LEFT))
            {
                selectedDifficultyIndex = std::max(0, selectedDifficultyIndex - 1);
            }
            else if (pointInRect(mouseX, mouseY, MENU_DIFF_RIGHT))
            {
                selectedDifficultyIndex = std::min(2, selectedDifficultyIndex + 1);
            }
            else if (pointInRect(mouseX, mouseY, MENU_START_BUTTON))
            {
                currentPlayerName = pendingPlayerName.empty() ? "PLAYER" : pendingPlayerName;
                resetGame();
            }
            else if (pointInRect(mouseX, mouseY, MENU_NAME_BOX))
            {
                SDL_StartTextInput();
            }
        }

        return;
    }

    if (sceneState == SceneState::Paused)
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_p)
            {
                sceneState = SceneState::Playing;
                Mix_ResumeMusic();
            }
            else if (event.key.keysym.sym == SDLK_m)
            {
                endRunToMenu();
            }
            else if (event.key.keysym.sym == SDLK_q)
            {
                requestQuitGame();
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            const int mouseX = event.button.x;
            const int mouseY = event.button.y;
            if (pointInRect(mouseX, mouseY, PAUSE_RESUME_BUTTON))
            {
                sceneState = SceneState::Playing;
                Mix_ResumeMusic();
            }
            else if (pointInRect(mouseX, mouseY, PAUSE_MENU_BUTTON))
            {
                endRunToMenu();
            }
            else if (pointInRect(mouseX, mouseY, PAUSE_EXIT_BUTTON))
            {
                requestQuitGame();
            }
        }
        return;
    }

    if (sceneState == SceneState::Playing && event.type == SDL_KEYDOWN &&
        (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_p))
    {
        sceneState = SceneState::Paused;
        Mix_PauseMusic();
        return;
    }

    if (sceneState == SceneState::GameOver && event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
        {
            sceneState = SceneState::Menu;
            gameStarted = false;
            gameOver = false;
            SDL_StartTextInput();
        }
    }
}

void GameScene::spawnPlayer()
{
    playerEntityId = registry.createEntity();
    registry.addComponent(playerEntityId, TransformComponent{Vector2(1280.0f, 720.0f), Vector2(1280.0f, 720.0f), 0.0f, 0.0f});
    registry.addComponent(playerEntityId, VelocityComponent{Vector2(), 0.0f, 250.0f, 200.0f, 0.055f, 160.0f});
    registry.addComponent(playerEntityId, ColliderComponent{28.0f, 40.0f, true, false, 1});
    registry.addComponent(playerEntityId, HealthComponent{100, 100, 0.0f, 1.5f, false});
    registry.addComponent(playerEntityId, PlayerStateComponent{
        0.0f,
        0.18f,
        0,
        0,
        false,
        0.0f,
        0.2f,
        0.0f,
        1.0f,
        Vector2(),
        false
    });
    registry.addComponent(playerEntityId, SpriteComponent{
        assetsRef->getTexture("player"),
        52,
        43,
        1,
        0,
        0.0f,
        1.0f,
        SDL_Color{255, 255, 255, 255}
    });
    registry.addComponent(playerEntityId, TagComponent{TagComponent::Tag::Player});

    cameraEntityId = registry.createEntity();
    registry.addComponent(cameraEntityId, ScreenshakeComponent{0.0f, 0.0f, 0.0f, Vector2()});
}

void GameScene::handleCollisions()
{
    const Vector2 playerPos = registry.isAlive(playerEntityId) && registry.hasComponent<TransformComponent>(playerEntityId)
                                  ? registry.getComponent<TransformComponent>(playerEntityId).position
                                  : Vector2(1280.0f, 720.0f);

    for (const PhysicsSystem::CollisionEvent& event : physics.getCollisions())
    {
        if (!registry.isAlive(event.a) || !registry.isAlive(event.b) ||
            !registry.hasComponent<TagComponent>(event.a) || !registry.hasComponent<TagComponent>(event.b))
        {
            continue;
        }

        const TagComponent::Tag tagA = registry.getComponent<TagComponent>(event.a).tag;
        const TagComponent::Tag tagB = registry.getComponent<TagComponent>(event.b).tag;

        auto handleBulletImpact = [&](std::uint32_t targetId)
        {
            if (!registry.isAlive(targetId) || !registry.hasComponent<TransformComponent>(targetId))
            {
                return;
            }

            const Vector2 hitPos = registry.getComponent<TransformComponent>(targetId).position;
            const TagComponent::Tag targetTag = registry.getComponent<TagComponent>(targetId).tag;
            if (targetTag == TagComponent::Tag::Enemy || targetTag == TagComponent::Tag::Boss)
            {
                triggerHitstop(registry, 0.04f);
                triggerHitFlash(registry, targetId, SDL_Color{255, 255, 255, 255}, 0.06f);
                spawnDeathParticles(registry, hitPos, SDL_Color{255, 180, 50, 255}, 8, 150.0f, 0.25f);
                playSpatialSound(*assetsRef, "explosion_sm", hitPos, playerPos);
            }
        };

        if (tagA == TagComponent::Tag::PlayerBullet)
        {
            handleBulletImpact(event.b);
        }
        else if (tagB == TagComponent::Tag::PlayerBullet)
        {
            handleBulletImpact(event.a);
        }

        const bool enemyBulletHitsPlayer = (tagA == TagComponent::Tag::EnemyBullet && tagB == TagComponent::Tag::Player) ||
                                           (tagB == TagComponent::Tag::EnemyBullet && tagA == TagComponent::Tag::Player);
        if (enemyBulletHitsPlayer &&
            registry.isAlive(playerEntityId) &&
            registry.hasComponent<HealthComponent>(playerEntityId) &&
            registry.getComponent<HealthComponent>(playerEntityId).iFrameTimer > 0.0f)
        {
            triggerScreenshake(registry, cameraEntityId, 5.0f, 0.3f);
            triggerHitstop(registry, 0.06f);
            triggerHitFlash(registry, playerEntityId, SDL_Color{255, 80, 80, 255}, 0.1f);
            playSpatialSound(*assetsRef, "player_hit", playerPos, playerPos, 0);
        }

        const bool pickupCollision = (tagA == TagComponent::Tag::Player && tagB == TagComponent::Tag::Pickup) ||
                                     (tagB == TagComponent::Tag::Player && tagA == TagComponent::Tag::Pickup);
        if (pickupCollision)
        {
            const std::uint32_t pickupId = (tagA == TagComponent::Tag::Pickup) ? event.a : event.b;
            if (registry.isAlive(playerEntityId) && registry.hasComponent<HealthComponent>(playerEntityId))
            {
                HealthComponent& health = registry.getComponent<HealthComponent>(playerEntityId);
                health.hp = std::min(health.hp + 30, health.maxHp);
            }

            if (registry.isAlive(pickupId) && registry.hasComponent<TransformComponent>(pickupId))
            {
                playSpatialSound(*assetsRef, "pickup", registry.getComponent<TransformComponent>(pickupId).position, playerPos);
                registry.destroyEntity(pickupId);
            }
        }
    }

    std::vector<std::uint32_t> enemiesToDestroy;
    for (const std::uint32_t entityId : registry.view<EnemyComponent, HealthComponent, TransformComponent>())
    {
        const HealthComponent& health = registry.getComponent<HealthComponent>(entityId);
        if (!health.dead)
        {
            continue;
        }

        const Vector2 enemyPos = registry.getComponent<TransformComponent>(entityId).position;
        spawnDeathParticles(registry, enemyPos, SDL_Color{255, 80, 30, 255}, 14, 200.0f, 0.4f);
        triggerHitstop(registry, 0.04f);
        playSpatialSound(*assetsRef, "explosion_sm", enemyPos, playerPos);
        if (randomFloat(0.0f, 1.0f) <= 0.1f)
        {
            spawnPickup(enemyPos);
        }
        enemiesToDestroy.push_back(entityId);
    }

    for (const std::uint32_t enemyId : enemiesToDestroy)
    {
        if (registry.isAlive(enemyId))
        {
            registry.destroyEntity(enemyId);
        }
    }

    if (boss.isDefeated(registry))
    {
        bool handledBossDeath = false;
        for (const std::uint32_t entityId : registry.view<TagComponent, TransformComponent>())
        {
            if (registry.getComponent<TagComponent>(entityId).tag != TagComponent::Tag::Boss)
            {
                continue;
            }

            const Vector2 bossPos = registry.getComponent<TransformComponent>(entityId).position;
            spawnDeathParticles(registry, bossPos, SDL_Color{255, 140, 0, 255}, 40, 300.0f, 0.8f);
            triggerScreenshake(registry, cameraEntityId, 18.0f, 1.2f);
            triggerHitstop(registry, 0.12f);
            playSpatialSound(*assetsRef, "explosion_lg", bossPos, playerPos);

            if (registry.isAlive(playerEntityId) && registry.hasComponent<PlayerStateComponent>(playerEntityId))
            {
                registry.getComponent<PlayerStateComponent>(playerEntityId).score += 2000 * waves.getCurrentWave();
            }

            registry.destroyEntity(entityId);
            boss.reset();
            handledBossDeath = true;
            break;
        }

        if (!handledBossDeath)
        {
            boss.reset();
        }
    }
}

void GameScene::checkGameOver()
{
    if (!registry.isAlive(playerEntityId) || !registry.hasComponent<HealthComponent>(playerEntityId))
    {
        return;
    }

    const HealthComponent& health = registry.getComponent<HealthComponent>(playerEntityId);
    if (health.hp <= 0 || health.dead)
    {
        if (gameOver)
        {
            return;
        }

        gameOver = true;
        sceneState = SceneState::GameOver;
        finalScore = registry.hasComponent<PlayerStateComponent>(playerEntityId)
                         ? registry.getComponent<PlayerStateComponent>(playerEntityId).score
                         : 0;
        gameOverTimer = 3.0f;
        submitScore();
    }
}

void GameScene::spawnPickup(Vector2 pos)
{
    const std::uint32_t entityId = registry.createEntity();
    registry.addComponent(entityId, TransformComponent{pos, pos, 0.0f, 0.0f});
    registry.addComponent(entityId, ColliderComponent{20.0f, 20.0f, false, true, 0});
    registry.addComponent(entityId, SpriteComponent{
        assetsRef->getTexture("pickup_health"),
        64,
        64,
        1,
        0,
        0.0f,
        1.0f,
        SDL_Color{255, 255, 255, 255}
    });
    registry.addComponent(entityId, TagComponent{TagComponent::Tag::Pickup});
}

void GameScene::resetGame()
{
    registry = Registry();
    physics = PhysicsSystem();
    renderSystem = RenderSystem();
    bullets = BulletSystem();
    ai = AISystem();
    boss.reset();
    waves.init();

    playerEntityId = 0;
    cameraEntityId = 0;
    gameOver = false;
    gameStarted = false;
    sceneState = SceneState::Playing;
    scoreSubmitted = false;
    gameOverTimer = 0.0f;
    finalScore = 0;
    worldHitstopEntityIdStorage() = 0U;

    SDL_StopTextInput();
    spawnPlayer();
    renderSystem.init(playerEntityId);
    renderSystem.setWaveInfo(1, false);

    switch (selectedDifficultyIndex)
    {
    case 0:
        waves.setDifficulty(WaveSystem::Difficulty::Easy);
        break;
    case 2:
        waves.setDifficulty(WaveSystem::Difficulty::Hard);
        break;
    default:
        waves.setDifficulty(WaveSystem::Difficulty::Medium);
        break;
    }

    waves.spawnWave(registry, *assetsRef, 1);
    Mix_PlayMusic(assetsRef->getMusic("bgm"), -1);
    Mix_VolumeMusic(45);
    gameStarted = true;
}

void GameScene::endRunToMenu()
{
    if (sceneState == SceneState::Playing || sceneState == SceneState::Paused)
    {
        finalScore = registry.isAlive(playerEntityId) && registry.hasComponent<PlayerStateComponent>(playerEntityId)
                         ? registry.getComponent<PlayerStateComponent>(playerEntityId).score
                         : finalScore;
        if (finalScore > 0)
        {
            submitScore();
        }
    }

    sceneState = SceneState::Menu;
    gameStarted = false;
    gameOver = false;
    scoreSubmitted = false;
    pendingPlayerName = currentPlayerName;
    SDL_StartTextInput();
    Mix_HaltMusic();
}

void GameScene::requestQuitGame()
{
    if (sceneState == SceneState::Playing || sceneState == SceneState::Paused)
    {
        finalScore = registry.isAlive(playerEntityId) && registry.hasComponent<PlayerStateComponent>(playerEntityId)
                         ? registry.getComponent<PlayerStateComponent>(playerEntityId).score
                         : finalScore;
        if (finalScore > 0)
        {
            submitScore();
        }
    }

    SDL_Event quitEvent{};
    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);
}

void GameScene::loadLeaderboard()
{
    leaderboard.clear();
    highestScore = 0;

    std::ifstream input("leaderboard.txt");
    if (!input.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream stream(line);
        std::string name;
        std::string scoreStr;
        std::string waveStr;
        std::string difficulty;
        if (!std::getline(stream, name, '|') ||
            !std::getline(stream, scoreStr, '|') ||
            !std::getline(stream, waveStr, '|') ||
            !std::getline(stream, difficulty, '|'))
        {
            continue;
        }

        leaderboard.push_back(ScoreEntry{name, std::stoi(scoreStr), std::stoi(waveStr), difficulty});
    }

    std::sort(
        leaderboard.begin(),
        leaderboard.end(),
        [](const ScoreEntry& a, const ScoreEntry& b)
        {
            return a.score > b.score;
        }
    );

    if (leaderboard.size() > 3U)
    {
        leaderboard.resize(3U);
    }

    if (!leaderboard.empty())
    {
        highestScore = leaderboard.front().score;
    }
}

void GameScene::saveLeaderboard() const
{
    std::ofstream output("leaderboard.txt", std::ios::trunc);
    if (!output.is_open())
    {
        return;
    }

    for (const ScoreEntry& entry : leaderboard)
    {
        output << entry.name << '|' << entry.score << '|' << entry.wave << '|' << entry.difficulty << '\n';
    }
}

void GameScene::submitScore()
{
    if (scoreSubmitted)
    {
        return;
    }

    leaderboard.push_back(ScoreEntry{
        currentPlayerName.empty() ? "PLAYER" : currentPlayerName,
        finalScore,
        waves.getCurrentWave(),
        getDifficultyLabel()
    });

    std::sort(
        leaderboard.begin(),
        leaderboard.end(),
        [](const ScoreEntry& a, const ScoreEntry& b)
        {
            return a.score > b.score;
        }
    );

    if (leaderboard.size() > 3U)
    {
        leaderboard.resize(3U);
    }

    highestScore = leaderboard.empty() ? 0 : leaderboard.front().score;
    saveLeaderboard();
    scoreSubmitted = true;
}

std::string GameScene::getDifficultyLabel() const
{
    switch (selectedDifficultyIndex)
    {
    case 0: return "Easy";
    case 2: return "Hard";
    default: return "Medium";
    }
}

void GameScene::update(float dt)
{
    if (sceneState == SceneState::Menu)
    {
        return;
    }

    if (sceneState == SceneState::Paused)
    {
        return;
    }

    if (!gameStarted)
    {
        return;
    }

    if (gameOver)
    {
        gameOverTimer = std::max(0.0f, gameOverTimer - dt);
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_R] != 0U)
        {
            resetGame();
        }
        return;
    }

    if (!isWorldHitstopActive(registry))
    {
        input.update(registry, playerEntityId, rendererRef, renderSystem.getCameraPos(), dt);

        if (registry.isAlive(playerEntityId) &&
            registry.hasComponent<PlayerStateComponent>(playerEntityId) &&
            registry.hasComponent<TransformComponent>(playerEntityId))
        {
            PlayerStateComponent& playerState = registry.getComponent<PlayerStateComponent>(playerEntityId);
            if (playerState.wantsToFire)
            {
                const TransformComponent& transform = registry.getComponent<TransformComponent>(playerEntityId);
                const Vector2 forward = Vector2(0.0f, -1.0f).rotated(transform.rotation);
                const Vector2 muzzleOffset = forward * 28.0f;
                bullets.spawnBullet(registry,
                                    transform.position + muzzleOffset,
                                    transform.rotation,
                                    520.0f,
                                    18.0f,
                                    true,
                                    assetsRef->getTexture("bullet_player"),
                                    950.0f);

                Mix_Chunk* shoot = assetsRef->getSound("shoot_player");
                if (shoot != nullptr)
                {
                    Mix_VolumeChunk(shoot, static_cast<int>(128.0f * 0.7f));
                    Mix_PlayChannel(-1, shoot, 0);
                }
                playerState.wantsToFire = false;
            }
        }

        ai.update(registry, physics.getQuadtree(), playerEntityId, selectedDifficultyIndex, bullets, *assetsRef, dt);
        boss.update(registry, playerEntityId, bullets, *assetsRef, dt);
    }

    physics.update(registry, dt);
    bullets.update(registry, physics.getCollisions(), dt);
    handleCollisions();
    waves.update(registry, bullets, boss, ai, *assetsRef, dt);
    renderSystem.setWaveInfo(waves.getCurrentWave(), waves.isBossWave());
    renderSystem.update(registry, dt);
    checkGameOver();
    physics.clearCollisions();
}

void GameScene::render(SDL_Renderer* renderer, AssetManager& assets, float alpha)
{
    if (sceneState == SceneState::Menu)
    {
        renderMenu(renderer, assets);
        return;
    }

    if (sceneState == SceneState::Paused)
    {
        renderPaused(renderer, assets, alpha);
        return;
    }

    if (gameOver)
    {
        renderGameOver(renderer, assets);
        return;
    }

    renderSystem.render(renderer, registry, assets, alpha);
    drawCenteredText(renderer, assets.getFont("hud"), "BEAT: " + std::to_string(highestScore), 640, 18, SDL_Color{220, 220, 220, 255});
}

void GameScene::renderMenu(SDL_Renderer* renderer, AssetManager& assets)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    drawCenteredText(renderer, assets.getFont("title"), "PRATSTRIKE", 640, 90, SDL_Color{255, 255, 255, 255});

    SDL_SetRenderDrawColor(renderer, 24, 30, 42, 220);
    SDL_RenderFillRect(renderer, &MENU_NAME_BOX);
    SDL_SetRenderDrawColor(renderer, 100, 180, 220, 255);
    SDL_RenderDrawRect(renderer, &MENU_NAME_BOX);
    drawCenteredText(renderer, assets.getFont("hud"), "NAME: " + (pendingPlayerName.empty() ? std::string("_") : pendingPlayerName), 640, 176, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "TYPE YOUR NAME OR CLICK THE BOX", 640, 224, SDL_Color{180, 180, 180, 255});

    SDL_SetRenderDrawColor(renderer, 24, 30, 42, 220);
    SDL_RenderFillRect(renderer, &MENU_DIFF_LEFT);
    SDL_RenderFillRect(renderer, &MENU_DIFF_RIGHT);
    SDL_SetRenderDrawColor(renderer, 100, 180, 220, 255);
    SDL_RenderDrawRect(renderer, &MENU_DIFF_LEFT);
    SDL_RenderDrawRect(renderer, &MENU_DIFF_RIGHT);
    drawCenteredText(renderer, assets.getFont("hud"), "<", MENU_DIFF_LEFT.x + (MENU_DIFF_LEFT.w / 2), MENU_DIFF_LEFT.y + 5, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), ">", MENU_DIFF_RIGHT.x + (MENU_DIFF_RIGHT.w / 2), MENU_DIFF_RIGHT.y + 5, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "DIFFICULTY: " + getDifficultyLabel(), 640, 276, SDL_Color{160, 240, 255, 255});

    SDL_SetRenderDrawColor(renderer, 44, 88, 52, 230);
    SDL_RenderFillRect(renderer, &MENU_START_BUTTON);
    SDL_SetRenderDrawColor(renderer, 180, 240, 180, 255);
    SDL_RenderDrawRect(renderer, &MENU_START_BUTTON);
    drawCenteredText(renderer, assets.getFont("hud"), "START NEW GAME", 640, 350, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "ENTER ALSO WORKS", 640, 404, SDL_Color{255, 220, 120, 255});

    drawCenteredText(renderer, assets.getFont("hud"), "TOP SCORE TO BEAT: " + std::to_string(highestScore), 640, 440, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "TOP 3 PILOTS", 640, 494, SDL_Color{255, 255, 255, 255});

    for (int index = 0; index < 3; ++index)
    {
        std::string line = std::to_string(index + 1) + ". ---";
        if (index < static_cast<int>(leaderboard.size()))
        {
            const ScoreEntry& entry = leaderboard[static_cast<std::size_t>(index)];
            line = std::to_string(index + 1) + ". " + entry.name + "  " +
                   std::to_string(entry.score) + "  " + entry.difficulty;
        }

        drawCenteredText(renderer, assets.getFont("hud"), line, 640, 534 + (index * 40), SDL_Color{230, 230, 230, 255});
    }
}

void GameScene::renderPaused(SDL_Renderer* renderer, AssetManager& assets, float alpha)
{
    renderSystem.render(renderer, registry, assets, alpha);
    drawCenteredText(renderer, assets.getFont("hud"), "BEAT: " + std::to_string(highestScore), 640, 18, SDL_Color{220, 220, 220, 255});

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    const SDL_Rect overlay{0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &overlay);

    drawCenteredText(renderer, assets.getFont("title"), "PAUSED", 640, 140, SDL_Color{255, 255, 255, 255});

    SDL_SetRenderDrawColor(renderer, 24, 30, 42, 230);
    SDL_RenderFillRect(renderer, &PAUSE_RESUME_BUTTON);
    SDL_RenderFillRect(renderer, &PAUSE_MENU_BUTTON);
    SDL_RenderFillRect(renderer, &PAUSE_EXIT_BUTTON);

    SDL_SetRenderDrawColor(renderer, 100, 180, 220, 255);
    SDL_RenderDrawRect(renderer, &PAUSE_RESUME_BUTTON);
    SDL_RenderDrawRect(renderer, &PAUSE_MENU_BUTTON);
    SDL_RenderDrawRect(renderer, &PAUSE_EXIT_BUTTON);

    drawCenteredText(renderer, assets.getFont("hud"), "RESUME", 640, 260, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "SAVE SCORE AND GO TO MENU", 640, 324, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "SAVE SCORE AND EXIT", 640, 388, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "ESC OR P RESUMES  |  M MENU  |  Q EXIT", 640, 470, SDL_Color{220, 220, 220, 255});
}

void GameScene::renderGameOver(SDL_Renderer* renderer, AssetManager& assets)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    const SDL_Rect overlay{0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &overlay);

    drawCenteredText(renderer, assets.getFont("title"), "GAME OVER", 640, 250, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "FINAL SCORE: " + std::to_string(finalScore), 640, 330, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "PRESS R TO RESTART", 640, 380, SDL_Color{255, 255, 255, 255});
    drawCenteredText(renderer, assets.getFont("hud"), "PRESS ENTER FOR MENU", 640, 420, SDL_Color{200, 200, 200, 255});
}
