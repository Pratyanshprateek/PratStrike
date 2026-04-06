#include "AssetManager.h"

#include <iostream>

namespace
{
    const char* getSdlStyleError(const char* fallback) noexcept
    {
        const char* error = SDL_GetError();
        return (error != nullptr && error[0] != '\0') ? error : fallback;
    }
}

AssetManager& AssetManager::get()
{
    static AssetManager instance;
    return instance;
}

SDL_Texture* AssetManager::loadTexture(const std::string& key, const std::string& path, SDL_Renderer* renderer)
{
    const auto found = textures.find(key);
    if (found != textures.end())
    {
        return found->second;
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    if (texture == nullptr)
    {
        std::cerr << "[AssetManager ERROR] Failed to load " << path << ": " << getSdlStyleError(IMG_GetError()) << '\n';
        return nullptr;
    }

    textures[key] = texture;
    return texture;
}

Mix_Chunk* AssetManager::loadSound(const std::string& key, const std::string& path)
{
    const auto found = sounds.find(key);
    if (found != sounds.end())
    {
        return found->second;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (chunk == nullptr)
    {
        std::cerr << "[AssetManager ERROR] Failed to load " << path << ": " << getSdlStyleError(Mix_GetError()) << '\n';
        return nullptr;
    }

    sounds[key] = chunk;
    return chunk;
}

Mix_Music* AssetManager::loadMusic(const std::string& key, const std::string& path)
{
    const auto found = music.find(key);
    if (found != music.end())
    {
        return found->second;
    }

    Mix_Music* track = Mix_LoadMUS(path.c_str());
    if (track == nullptr)
    {
        std::cerr << "[AssetManager ERROR] Failed to load " << path << ": " << getSdlStyleError(Mix_GetError()) << '\n';
        return nullptr;
    }

    music[key] = track;
    return track;
}

TTF_Font* AssetManager::loadFont(const std::string& key, const std::string& path, int ptsize)
{
    const auto found = fonts.find(key);
    if (found != fonts.end())
    {
        return found->second;
    }

    TTF_Font* font = TTF_OpenFont(path.c_str(), ptsize);
    if (font == nullptr)
    {
        std::cerr << "[AssetManager ERROR] Failed to load " << path << ": " << getSdlStyleError(TTF_GetError()) << '\n';
        return nullptr;
    }

    fonts[key] = font;
    return font;
}

SDL_Texture* AssetManager::getTexture(const std::string& key)
{
    const auto found = textures.find(key);
    return (found != textures.end()) ? found->second : nullptr;
}

Mix_Chunk* AssetManager::getSound(const std::string& key)
{
    const auto found = sounds.find(key);
    return (found != sounds.end()) ? found->second : nullptr;
}

Mix_Music* AssetManager::getMusic(const std::string& key)
{
    const auto found = music.find(key);
    return (found != music.end()) ? found->second : nullptr;
}

TTF_Font* AssetManager::getFont(const std::string& key)
{
    const auto found = fonts.find(key);
    return (found != fonts.end()) ? found->second : nullptr;
}

void AssetManager::cleanup()
{
    for (auto& entry : textures)
    {
        SDL_DestroyTexture(entry.second);
    }
    textures.clear();

    for (auto& entry : sounds)
    {
        Mix_FreeChunk(entry.second);
    }
    sounds.clear();

    for (auto& entry : music)
    {
        Mix_FreeMusic(entry.second);
    }
    music.clear();

    for (auto& entry : fonts)
    {
        TTF_CloseFont(entry.second);
    }
    fonts.clear();
}
