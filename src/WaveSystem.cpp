#include "WaveSystem.h"

WaveSystem::WaveSystem()
    : currentWave(0),
      enemiesAlive(0),
      betweenWaveTimer(0.0f),
      betweenWaveDuration(4.0f),
      waitingForNextWave(false),
      bossWaveActive(false),
      difficulty(Difficulty::Medium),
      waveTable()
{
}

void WaveSystem::init()
{
    currentWave = 0;
    enemiesAlive = 0;
    betweenWaveTimer = 0.0f;
    waitingForNextWave = false;
    bossWaveActive = false;
    difficulty = Difficulty::Medium;
    waveTable.clear();

    waveTable.push_back({5, 0, 0, 0, false});
    waveTable.push_back({6, 2, 0, 0, false});
    waveTable.push_back({5, 3, 1, 0, false});
    waveTable.push_back({7, 3, 1, 1, false});
    waveTable.push_back({0, 0, 0, 0, true});
    waveTable.push_back({8, 4, 2, 2, false});
    waveTable.push_back({10, 4, 2, 2, false});
    waveTable.push_back({8, 6, 3, 3, false});
    waveTable.push_back({10, 6, 3, 3, false});
    waveTable.push_back({0, 0, 0, 0, true});
}

void WaveSystem::setDifficulty(Difficulty newDifficulty)
{
    difficulty = newDifficulty;
}

WaveSystem::WaveConfig WaveSystem::getWaveConfig(int waveNumber) const
{
    if (waveNumber >= 1 && waveNumber <= static_cast<int>(waveTable.size()))
    {
        return waveTable[static_cast<std::size_t>(waveNumber - 1)];
    }

    return WaveConfig{
        4 + waveNumber,
        std::max(1, waveNumber / 2),
        std::max(0, (waveNumber - 3) / 3),
        std::max(0, (waveNumber - 4) / 3),
        (waveNumber % 5) == 0
    };
}

int WaveSystem::scaleEnemyCount(int baseCount) const
{
    if (baseCount <= 0)
    {
        return 0;
    }

    float multiplier = 1.0f;
    switch (difficulty)
    {
    case Difficulty::Easy:
        multiplier = 0.75f;
        break;
    case Difficulty::Medium:
        multiplier = 1.0f;
        break;
    case Difficulty::Hard:
        multiplier = 1.35f;
        break;
    }

    const int scaled = static_cast<int>(baseCount * multiplier + 0.5f);
    return std::max(1, scaled);
}

const char* WaveSystem::getDifficultyName() const
{
    switch (difficulty)
    {
    case Difficulty::Easy: return "Easy";
    case Difficulty::Medium: return "Medium";
    case Difficulty::Hard: return "Hard";
    }

    return "Medium";
}

Vector2 WaveSystem::chooseSpawnPoint(Registry& reg) const
{
    Vector2 playerPos(1280.0f, 720.0f);
    for (const std::uint32_t entityId : reg.view<TagComponent, TransformComponent>())
    {
        if (reg.getComponent<TagComponent>(entityId).tag == TagComponent::Tag::Player)
        {
            playerPos = reg.getComponent<TransformComponent>(entityId).position;
            break;
        }
    }

    const float left = clamp(playerPos.x - 640.0f - 100.0f, 0.0f, 2560.0f);
    const float right = clamp(playerPos.x + 640.0f + 100.0f, 0.0f, 2560.0f);
    const float top = clamp(playerPos.y - 360.0f - 100.0f, 0.0f, 1440.0f);
    const float bottom = clamp(playerPos.y + 360.0f + 100.0f, 0.0f, 1440.0f);

    switch (randomInt(0, 3))
    {
    case 0: return Vector2(randomFloat(left, right), top);
    case 1: return Vector2(randomFloat(left, right), bottom);
    case 2: return Vector2(left, randomFloat(top, bottom));
    default: return Vector2(right, randomFloat(top, bottom));
    }
}

std::uint32_t WaveSystem::spawnEnemy(Registry& reg,
                                     AssetManager& assets,
                                     EnemyComponent::EnemyType type,
                                     Vector2 worldPos)
{
    const std::uint32_t entityId = reg.createEntity();
    SDL_Texture* texture = nullptr;
    int hp = 60;
    int frameW = 52;
    int frameH = 43;

    switch (type)
    {
    case EnemyComponent::EnemyType::Grunt:
        texture = assets.getTexture("enemy_grunt");
        hp = 60;
        frameW = 52;
        frameH = 43;
        break;
    case EnemyComponent::EnemyType::Flanker:
        texture = assets.getTexture("enemy_flanker");
        hp = 45;
        frameW = 49;
        frameH = 43;
        break;
    case EnemyComponent::EnemyType::Tank:
        texture = assets.getTexture("enemy_tank");
        hp = 180;
        frameW = 49;
        frameH = 43;
        break;
    case EnemyComponent::EnemyType::Sniper:
        texture = assets.getTexture("enemy_sniper");
        hp = 50;
        frameW = 44;
        frameH = 43;
        break;
    }

    reg.addComponent(entityId, TransformComponent{worldPos, worldPos, 180.0f, 180.0f});
    reg.addComponent(entityId, VelocityComponent{Vector2(), 0.0f, 120.0f, 180.0f, 0.04f, 200.0f});
    reg.addComponent(entityId, ColliderComponent{28.0f, 40.0f, true, false, 2});
    reg.addComponent(entityId, HealthComponent{hp, hp, 0.0f, 0.12f, false});
    reg.addComponent(entityId, SpriteComponent{
        texture,
        frameW,
        frameH,
        1,
        0,
        0.0f,
        1.0f,
        SDL_Color{255, 255, 255, 255}
    });
    reg.addComponent(entityId, TagComponent{TagComponent::Tag::Enemy});
    reg.addComponent(entityId, EnemyComponent{
        type,
        280.0f,
        250.0f,
        randomFloat(0.1f, 0.6f),
        1.2f,
        EnemyComponent::AIState::Patrol,
        Vector2(randomFloat(200.0f, 2360.0f), randomFloat(200.0f, 1240.0f)),
        randomFloat(0.5f, 2.5f),
        100
    });
    return entityId;
}

void WaveSystem::spawnWave(Registry& reg, AssetManager& assets, int waveNumber)
{
    currentWave = waveNumber;
    const WaveConfig config = getWaveConfig(waveNumber);
    bossWaveActive = config.hasBoss;

    for (int i = 0; i < scaleEnemyCount(config.grunts); ++i)
    {
        spawnEnemy(reg, assets, EnemyComponent::EnemyType::Grunt, chooseSpawnPoint(reg));
    }
    for (int i = 0; i < scaleEnemyCount(config.flankers); ++i)
    {
        spawnEnemy(reg, assets, EnemyComponent::EnemyType::Flanker, chooseSpawnPoint(reg));
    }
    for (int i = 0; i < scaleEnemyCount(config.tanks); ++i)
    {
        spawnEnemy(reg, assets, EnemyComponent::EnemyType::Tank, chooseSpawnPoint(reg));
    }
    for (int i = 0; i < scaleEnemyCount(config.snipers); ++i)
    {
        spawnEnemy(reg, assets, EnemyComponent::EnemyType::Sniper, chooseSpawnPoint(reg));
    }
}

void WaveSystem::update(Registry& reg,
                        BulletSystem& bullets,
                        BossSystem& boss,
                        AISystem& ai,
                        AssetManager& assets,
                        float dt)
{
    (void)bullets;
    (void)ai;

    enemiesAlive = static_cast<int>(reg.view<EnemyComponent>().size()) + (boss.isActive() ? 1 : 0);
    if (enemiesAlive <= 0 && !waitingForNextWave)
    {
        waitingForNextWave = true;
        betweenWaveTimer = betweenWaveDuration;
    }

    if (!waitingForNextWave)
    {
        return;
    }

    betweenWaveTimer -= dt;
    if (betweenWaveTimer > 0.0f)
    {
        return;
    }

    waitingForNextWave = false;
    const int nextWave = currentWave + 1;
    const WaveConfig config = getWaveConfig(nextWave);
    spawnWave(reg, assets, nextWave);
    if (config.hasBoss)
    {
        boss.spawnBoss(reg, assets, Vector2(1280.0f, 420.0f));
    }
}
