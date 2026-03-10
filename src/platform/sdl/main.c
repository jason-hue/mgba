#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/vfs.h>
#include <SDL.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

struct mSDLRenderer {
    struct mCore* core;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t* pixels;
    int width;
    int height;
};

static void mSDLRun(struct mSDLRenderer* renderer) {
    renderer->core->setAudioBufferSize(renderer->core, 2048);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        const uint8_t* state = SDL_GetKeyboardState(NULL);
        uint16_t keys = 0;
        if (state[SDL_SCANCODE_X]) keys |= (1 << GBA_KEY_A);
        if (state[SDL_SCANCODE_Z]) keys |= (1 << GBA_KEY_B);
        if (state[SDL_SCANCODE_RETURN]) keys |= (1 << GBA_KEY_START);
        if (state[SDL_SCANCODE_BACKSPACE]) keys |= (1 << GBA_KEY_SELECT);
        if (state[SDL_SCANCODE_UP]) keys |= (1 << GBA_KEY_UP);
        if (state[SDL_SCANCODE_DOWN]) keys |= (1 << GBA_KEY_DOWN);
        if (state[SDL_SCANCODE_LEFT]) keys |= (1 << GBA_KEY_LEFT);
        if (state[SDL_SCANCODE_RIGHT]) keys |= (1 << GBA_KEY_RIGHT);
        if (state[SDL_SCANCODE_S]) keys |= (1 << GBA_KEY_R);
        if (state[SDL_SCANCODE_A]) keys |= (1 << GBA_KEY_L);
        
        renderer->core->setKeys(renderer->core, keys);
        renderer->core->runFrame(renderer->core);

        // Update texture with ABGR8888 format
        SDL_UpdateTexture(renderer->texture, NULL, renderer->pixels, renderer->width * sizeof(uint32_t));
        SDL_RenderClear(renderer->renderer);
        SDL_RenderCopy(renderer->renderer, renderer->texture, NULL, NULL);
        SDL_RenderPresent(renderer->renderer);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <game.gba>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    struct mSDLRenderer renderer = {0};
    renderer.width = 240;
    renderer.height = 160;
    renderer.pixels = calloc(renderer.width * renderer.height, sizeof(uint32_t));

    renderer.window = SDL_CreateWindow("mGBA Minimal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                     renderer.width * 3, renderer.height * 3, SDL_WINDOW_SHOWN);
    renderer.renderer = SDL_CreateRenderer(renderer.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // 改为 ABGR8888 以匹配 mGBA 的内存布局
    renderer.texture = SDL_CreateTexture(renderer.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 
                                       renderer.width, renderer.height);

    renderer.core = GBACoreCreate();
    if (!renderer.core) {
        printf("Failed to create GBA core.\n");
        return 1;
    }

    if (!renderer.core->init(renderer.core)) {
        printf("Failed to initialize GBA core.\n");
        return 1;
    }

    mCoreInitConfig(renderer.core, "sdl");
    renderer.core->setVideoBuffer(renderer.core, (mColor*)renderer.pixels, renderer.width);

    struct VFile* rom = VFileOpenFD(argv[1], O_RDONLY);
    if (!rom) {
        printf("Could not open ROM: %s\n", argv[1]);
        renderer.core->deinit(renderer.core);
        return 1;
    }

    if (!renderer.core->loadROM(renderer.core, rom)) {
        printf("Failed to load ROM.\n");
        renderer.core->deinit(renderer.core);
        return 1;
    }

    renderer.core->reset(renderer.core);
    mSDLRun(&renderer);

    renderer.core->deinit(renderer.core);
    SDL_DestroyTexture(renderer.texture);
    SDL_DestroyRenderer(renderer.renderer);
    SDL_DestroyWindow(renderer.window);
    free(renderer.pixels);
    SDL_Quit();

    return 0;
}
