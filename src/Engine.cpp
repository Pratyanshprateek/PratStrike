#include "Engine.h"

#include <iostream>

Engine::Engine()
    : window(nullptr),
      renderer(nullptr),
      running(false),
      initialized(false),
      lastTick(0),
      accumulator(0.0f)
{
}

bool Engine::init(const char* title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }

    const int imageFlags = IMG_INIT_PNG;
    if ((IMG_Init(imageFlags) & imageFlags) != imageFlags)
    {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << '\n';
        SDL_Quit();
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0)
    {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << '\n';
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    if (TTF_Init() != 0)
    {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << '\n';
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (renderer == nullptr)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    running = true;
    initialized = true;
    lastTick = SDL_GetTicks64();
    accumulator = 0.0f;

    onInit();
    return true;
}

void Engine::run()
{
    while (running)
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0)
        {
            onEvent(event);

            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        const std::uint64_t currentTick = SDL_GetTicks64();
        float frameTime = static_cast<float>(currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        if (frameTime > MAX_FRAME_TIME)
        {
            frameTime = MAX_FRAME_TIME;
        }

        accumulator += frameTime;

        while (accumulator >= FIXED_DT)
        {
            onUpdate(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        const float alpha = accumulator / FIXED_DT;

        SDL_SetRenderDrawColor(renderer, 15, 15, 25, 255);
        SDL_RenderClear(renderer);

        onRender(alpha);
        SDL_RenderPresent(renderer);
    }
}

void Engine::shutdown()
{
    if (initialized)
    {
        onShutdown();
    }

    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();

    running = false;
    initialized = false;
    lastTick = 0;
    accumulator = 0.0f;
}

SDL_Renderer* Engine::getRenderer()
{
    return renderer;
}
