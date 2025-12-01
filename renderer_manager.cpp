#include "renderer_manager.h"
#include <iostream>

RendererManager::RendererManager() 
    : controlTarget{nullptr, nullptr, nullptr, 0, 0, false, {}}
    , outputTarget{nullptr, nullptr, nullptr, 0, 0, false, {}}
    , controlNeedsRedraw(true)
    , outputNeedsRedraw(true)
    , lastFrameTime(std::chrono::steady_clock::now()) {
}

RendererManager::~RendererManager() {
    destroyTarget(controlTarget);
    destroyTarget(outputTarget);
}

bool RendererManager::initialize(int windowWidth, int windowHeight) {
    if (!initializeTarget(controlTarget, "ChieModel Control", windowWidth, windowHeight)) {
        return false;
    }
    
    if (!initializeTarget(outputTarget, "ChieModel Output", windowWidth, windowHeight)) {
        destroyTarget(controlTarget);
        return false;
    }
    
    return true;
}

bool RendererManager::initializeTarget(RenderTarget& target, const char* title, int width, int height) {
    target.window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   width, height,
                                   SDL_WINDOW_SHOWN);
    if (!target.window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    
    target.renderer = SDL_CreateRenderer(target.window, -1, 
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!target.renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(target.window);
        target.window = nullptr;
        return false;
    }
    
    target.width = width;
    target.height = height;
    target.needsUpdate = true;
    target.lastUpdate = std::chrono::steady_clock::now();
    
    // Create backbuffer for double buffering
    target.backbuffer = SDL_CreateTexture(target.renderer, 
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         width, height);
    if (!target.backbuffer) {
        std::cerr << "Failed to create backbuffer: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(target.renderer);
        SDL_DestroyWindow(target.window);
        target.renderer = nullptr;
        target.window = nullptr;
        return false;
    }
    
    return true;
}

void RendererManager::destroyTarget(RenderTarget& target) {
    if (target.backbuffer) {
        SDL_DestroyTexture(target.backbuffer);
        target.backbuffer = nullptr;
    }
    
    if (target.renderer) {
        SDL_DestroyRenderer(target.renderer);
        target.renderer = nullptr;
    }
    
    if (target.window) {
        SDL_DestroyWindow(target.window);
        target.window = nullptr;
    }
}

void RendererManager::present() {
    auto now = std::chrono::steady_clock::now();
    
    if (controlNeedsRedraw) {
        SDL_RenderPresent(controlTarget.renderer);
        controlNeedsRedraw = false;
        controlTarget.lastUpdate = now;
    }
    
    if (outputNeedsRedraw) {
        SDL_RenderPresent(outputTarget.renderer);
        outputNeedsRedraw = false;
        outputTarget.lastUpdate = now;
    }
}

void RendererManager::waitForNextFrame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
    
    if (elapsed < frameInterval) {
        SDL_Delay(static_cast<Uint32>((frameInterval - elapsed).count()));
    }
    
    lastFrameTime = std::chrono::steady_clock::now();
}

void RendererManager::renderTextureToTarget(RenderTarget* target, SDL_Texture* texture, bool flipped) {
    if (!target || !target->renderer || !texture) {
        return;
    }
    
    // Set render target to backbuffer
    if (SDL_SetRenderTarget(target->renderer, target->backbuffer) != 0) {
        std::cerr << "Failed to set render target: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Clear with green background
    SDL_SetRenderDrawColor(target->renderer, 0, 255, 0, 255);
    SDL_RenderClear(target->renderer);
    
    // Get texture dimensions
    int texWidth, texHeight;
    SDL_QueryTexture(texture, nullptr, nullptr, &texWidth, &texHeight);
    
    // Calculate position to center the texture
    SDL_Rect destRect;
    destRect.w = texWidth;
    destRect.h = texHeight;
    destRect.x = (target->width - texWidth) / 2;
    destRect.y = (target->height - texHeight) / 2;
    
    // Render with optional flip
    if (flipped) {
        SDL_RenderCopyEx(target->renderer, texture, nullptr, &destRect, 0, nullptr, SDL_FLIP_HORIZONTAL);
    } else {
        SDL_RenderCopy(target->renderer, texture, nullptr, &destRect);
    }
    
    // Reset render target
    SDL_SetRenderTarget(target->renderer, nullptr);
    
    // Copy backbuffer to screen
    SDL_RenderCopy(target->renderer, target->backbuffer, nullptr, nullptr);
    
    // Mark as updated
    if (target == &controlTarget) {
        controlNeedsRedraw = true;
    } else if (target == &outputTarget) {
        outputNeedsRedraw = true;
    }
}

void RendererManager::clearTarget(RenderTarget* target, const SDL_Color& color) {
    if (!target || !target->renderer) {
        return;
    }
    
    SDL_SetRenderDrawColor(target->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(target->renderer);
    
    if (target == &controlTarget) {
        controlNeedsRedraw = true;
    } else if (target == &outputTarget) {
        outputNeedsRedraw = true;
    }
}