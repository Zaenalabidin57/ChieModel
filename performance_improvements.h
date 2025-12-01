// Simple performance improvements for the existing ChieModel.cpp
// This provides drop-in replacements for key functions

#ifndef PERFORMANCE_IMPROVEMENTS_H
#define PERFORMANCE_IMPROVEMENTS_H

#include <SDL2/SDL.h>
#include <map>
#include <memory>

// Simple texture cache to avoid recreation
class TextureCache {
private:
    std::map<SDL_Texture*, SDL_Texture*> cache;
    SDL_Texture* lastOutputTexture = nullptr;
    bool needsUpdate = true;
    
public:
    void markNeedsUpdate() { needsUpdate = true; }
    
    SDL_Texture* getOrCreateOutputTexture(SDL_Renderer* renderer, SDL_Surface* surface) {
        if (!needsUpdate && lastOutputTexture) {
            return lastOutputTexture;
        }
        
        if (lastOutputTexture) {
            SDL_DestroyTexture(lastOutputTexture);
        }
        
        lastOutputTexture = SDL_CreateTextureFromSurface(renderer, surface);
        needsUpdate = false;
        return lastOutputTexture;
    }
    
    ~TextureCache() {
        if (lastOutputTexture) {
            SDL_DestroyTexture(lastOutputTexture);
        }
    }
};

// Simple frame rate controller
class FrameRateController {
private:
    Uint32 lastFrameTime = 0;
    const Uint32 targetFrameTime = 16; // ~60 FPS
    
public:
    bool shouldUpdate() {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastFrameTime >= targetFrameTime) {
            lastFrameTime = currentTime;
            return true;
        }
        return false;
    }
    
    void delay() {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsed = currentTime - lastFrameTime;
        if (elapsed < targetFrameTime) {
            SDL_Delay(targetFrameTime - elapsed);
        }
    }
};

#endif // PERFORMANCE_IMPROVEMENTS_H