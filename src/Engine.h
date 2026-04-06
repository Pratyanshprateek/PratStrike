#pragma once

#include <cstdint>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

class Engine
{
public:
    Engine();
    virtual ~Engine() = default;

    bool init(const char* title, int width, int height);
    void run();
    void shutdown();
    SDL_Renderer* getRenderer();

protected:
    virtual void onInit() {}
    virtual void onEvent(const SDL_Event& event) { (void)event; }
    virtual void onUpdate(float dt) { (void)dt; }
    virtual void onRender(float alpha) { (void)alpha; }
    virtual void onShutdown() {}

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    bool initialized;
    std::uint64_t lastTick;
    float accumulator;

    static constexpr float FIXED_DT = 1.0f / 60.0f;
    static constexpr float MAX_FRAME_TIME = 0.05f;
};
