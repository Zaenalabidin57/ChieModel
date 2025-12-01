#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include <SDL2/SDL.h>
#include <memory>
#include <chrono>

class RendererManager {
public:
    struct RenderTarget {
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* backbuffer;
        int width, height;
        bool needsUpdate;
        std::chrono::steady_clock::time_point lastUpdate;
    };

private:
    RenderTarget controlTarget;
    RenderTarget outputTarget;
    
    // Performance optimization: only render when state changes
    bool controlNeedsRedraw;
    bool outputNeedsRedraw;
    
    // Frame rate limiting
    std::chrono::steady_clock::time_point lastFrameTime;
    const std::chrono::milliseconds frameInterval{16}; // ~60 FPS
    
public:
    RendererManager();
    ~RendererManager();
    
    // Initialize both renderers
    bool initialize(int windowWidth, int windowHeight);
    
    // Get render targets
    RenderTarget* getControlTarget() { return &controlTarget; }
    RenderTarget* getOutputTarget() { return &outputTarget; }
    
    // Mark render targets as needing update
    void markControlNeedsRedraw() { controlNeedsRedraw = true; }
    void markOutputNeedsRedraw() { outputNeedsRedraw = true; }
    
    // Present renderers (only if needed)
    void present();
    
    // Frame rate limiting
    void waitForNextFrame();
    
    // Render texture to target
    void renderTextureToTarget(RenderTarget* target, SDL_Texture* texture, bool flipped = false);
    
    // Clear target
    void clearTarget(RenderTarget* target, const SDL_Color& color);
    
    // Check if windows are valid
    bool isValid() const {
        return controlTarget.window && outputTarget.window &&
               controlTarget.renderer && outputTarget.renderer;
    }
    
private:
    bool initializeTarget(RenderTarget& target, const char* title, int width, int height);
    void destroyTarget(RenderTarget& target);
};

#endif // RENDERER_MANAGER_H