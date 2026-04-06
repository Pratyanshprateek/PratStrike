#pragma once

#include <vector>

#include "AISystem.h"
#include "AssetManager.h"
#include "BossSystem.h"
#include "BulletSystem.h"
#include "Components.h"
#include "ECS.h"

class WaveSystem
{
public:
    enum class Difficulty
    {
        Easy,
        Medium,
        Hard
    };

    struct WaveConfig
    {
        int grunts;
        int flankers;
        int tanks;
        int snipers;
        bool hasBoss;
    };

    WaveSystem();

    void init();
    void setDifficulty(Difficulty newDifficulty);
    void update(Registry& reg,
                BulletSystem& bullets,
                BossSystem& boss,
                AISystem& ai,
                AssetManager& assets,
                float dt);
    void spawnWave(Registry& reg, AssetManager& assets, int waveNumber);
    std::uint32_t spawnEnemy(Registry& reg,
                             AssetManager& assets,
                             EnemyComponent::EnemyType type,
                             Vector2 worldPos);
    int getCurrentWave() const { return currentWave; }
    int getEnemiesAlive() const { return enemiesAlive; }
    bool isBossWave() const { return bossWaveActive; }
    Difficulty getDifficulty() const { return difficulty; }
    const char* getDifficultyName() const;

private:
    WaveConfig getWaveConfig(int waveNumber) const;
    int scaleEnemyCount(int baseCount) const;
    Vector2 chooseSpawnPoint(Registry& reg) const;

    int currentWave;
    int enemiesAlive;
    float betweenWaveTimer;
    float betweenWaveDuration;
    bool waitingForNextWave;
    bool bossWaveActive;
    Difficulty difficulty;
    std::vector<WaveConfig> waveTable;
};
