#ifndef OPTIMIZED_AVATAR_SYSTEM_H
#define OPTIMIZED_AVATAR_SYSTEM_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <memory>
#include <map>
#include <string>
#include <chrono>

#include "resource_manager.h"
#include "renderer_manager.h"
#include "animation_system.h"

class OptimizedAvatarSystem {
public:
    struct PoseExpression {
        int pose;
        int expression;
        PoseExpression(int p, int e) : pose(p), expression(e) {}
    };

private:
    // Core components
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<RendererManager> rendererManager;
    std::unique_ptr<AnimationSystem> animationSystem;
    
    // UI components
    TTF_Font* font;
    
    // State management
    int currentPose;
    int currentExpression;
    bool isFlipped;
    SDL_Keycode lastKey;
    std::chrono::steady_clock::time_point lastKeyTime;
    
    // Configuration
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const std::chrono::milliseconds KEY_COOLDOWN{100};
    const SDL_Color BACKGROUND_COLOR = {0, 255, 0, 255};
    
    // Poses and key mappings
    std::map<int, std::string> poses;
    std::map<SDL_Keycode, PoseExpression> keyMappings;
    
public:
    OptimizedAvatarSystem();
    ~OptimizedAvatarSystem();
    
    bool initialize(const std::string& modelDirectory);
    void run();
    void shutdown();
    
private:
    bool initializeSDL();
    bool initializeFonts();
    void setupMappings();
    
    // Event handling
    void handleKeyPress(SDL_Keycode key);
    bool shouldProcessKey(SDL_Keycode key);
    
    // Rendering
    void renderControlPanel();
    void renderOutputWindow();
    void render();
    void renderUIElements();
    
    // Animation
    void updateAnimations();
    
    // Utility
    std::string getModelDirectory();
    SDL_Texture* getCurrentTexture();
    int getEffectiveExpression();
};

#endif // OPTIMIZED_AVATAR_SYSTEM_H