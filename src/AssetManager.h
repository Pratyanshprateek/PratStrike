#pragma once

#include <string>
#include <unordered_map>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

class AssetManager
{
public:
    static AssetManager& get();

    SDL_Texture* loadTexture(const std::string& key, const std::string& path, SDL_Renderer* renderer);
    Mix_Chunk* loadSound(const std::string& key, const std::string& path);
    Mix_Music* loadMusic(const std::string& key, const std::string& path);
    TTF_Font* loadFont(const std::string& key, const std::string& path, int ptsize);

    SDL_Texture* getTexture(const std::string& key);
    Mix_Chunk* getSound(const std::string& key);
    Mix_Music* getMusic(const std::string& key);
    TTF_Font* getFont(const std::string& key);

    void cleanup();

private:
    AssetManager() = default;
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, Mix_Chunk*> sounds;
    std::unordered_map<std::string, Mix_Music*> music;
    std::unordered_map<std::string, TTF_Font*> fonts;
};
