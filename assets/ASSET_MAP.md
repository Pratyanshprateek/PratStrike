# PratStrike Asset Map

This folder is now split into:

- `sprites/`: runtime-ready PNGs with stable game-facing names
- `audio/`: runtime-ready sound files with stable game-facing names
- `fonts/`: runtime-ready font files
- `_source_raw/`: original downloaded packs kept out of the way
- `_unused/`: reserved for future leftovers or experiments

## Sprite mapping

- `player.png`: curated from Kenney top-down shooter `soldier1_machine.png`
- `enemy_grunt.png`: curated from `zoimbie1_machine.png`
- `enemy_flanker.png`: curated from `manBlue_machine.png`
- `enemy_tank.png`: curated from `robot1_machine.png`
- `enemy_sniper.png`: curated from `hitman1_silencer.png`
- `boss.png`: enlarged from `robot1_machine.png` as a temporary boss placeholder
- `bullet_player.png`: curated from `weapon_silencer.png`
- `bullet_enemy.png`: curated from `weapon_gun.png`
- `pickup_health.png`: curated from tile `tile_210.png`
- `wall_tile.png`: curated from tile `tile_01.png`

## Audio mapping

- `shoot_player.wav`: converted from `laserSmall_000.ogg`
- `shoot_enemy.wav`: converted from `laserRetro_001.ogg`
- `explosion_small.wav`: converted from `explosionCrunch_001.ogg`
- `explosion_large.wav`: converted from `lowFrequency_explosion_000.ogg`
- `player_hit.wav`: converted from `impactMetal_002.ogg`
- `pickup.wav`: converted from `confirmation_002.ogg`
- `boss_roar.wav`: converted from `computerNoise_001.ogg`
- `bgm.ogg`: temporary placeholder copied from `spaceEngineLow_001.ogg`

## Note

The downloaded packs did not include a true music loop or ship/tank sprite set. The current layout is ready for development, and the temporary placeholders can be swapped later without changing code paths.
