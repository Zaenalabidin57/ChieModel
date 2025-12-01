#include "optimized_avatar_system.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

OptimizedAvatarSystem::OptimizedAvatarSystem()
    : font(nullptr)
    , currentPose(1), currentExpression(1), isFlipped(false)
    , lastKey(0), lastKeyTime(std::chrono::steady_clock::now()) {
}

OptimizedAvatarSystem::~OptimizedAvatarSystem() {
    shutdown();
}

bool OptimizedAvatarSystem::initialize(const std::string& modelDirectory) {
    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Setup mappings
    setupMappings();
    
    // Initialize SDL
    if (!initializeSDL()) {
        return false;
    }
    
    // Initialize fonts
    if (!initializeFonts()) {
        std::cerr << "Warning: Failed to initialize fonts, continuing without text rendering" << std::endl;
    }
    
    // Initialize resource manager
    resourceManager = std::make_unique<ResourceManager>();
    if (!resourceManager->initialize(modelDirectory)) {
        std::cerr << "Failed to initialize resource manager" << std::endl;
        return false;
    }
    
    // Initialize renderer manager
    rendererManager = std::make_unique<RendererManager>();
    if (!rendererManager->initialize(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize renderer manager" << std::endl;
        return false;
    }
    
    // Initialize animation system
    animationSystem = std::make_unique<AnimationSystem>(resourceManager.get());
    
    std::cout << "Optimized Avatar System initialized successfully" << std::endl;
    return true;
}

bool OptimizedAvatarSystem::initializeSDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }
    
    // Initialize TTF
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    return true;
}

bool OptimizedAvatarSystem::initializeFonts() {
    font = TTF_OpenFont("fonts/FreeMono.otf", 16);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

void OptimizedAvatarSystem::setupMappings() {
    // Pose definitions
    poses = {
        {1, "santai"},
        {3, "satu tangan"},
        {4, "belakang tangan"},
        {6, "wawa"}
    };
    
    // Key mappings
    keyMappings = {
        {SDLK_q, PoseExpression(1, 2)},
        {SDLK_a, PoseExpression(1, 3)},
        {SDLK_z, PoseExpression(1, 4)},
        {SDLK_w, PoseExpression(3, 2)},
        {SDLK_s, PoseExpression(3, 3)},
        {SDLK_x, PoseExpression(3, 4)},
        {SDLK_e, PoseExpression(4, 2)},
        {SDLK_d, PoseExpression(4, 3)},
        {SDLK_c, PoseExpression(4, 4)},
        {SDLK_r, PoseExpression(6, 2)},
        {SDLK_f, PoseExpression(6, 3)},
        {SDLK_v, PoseExpression(6, 4)},
    };
}

void OptimizedAvatarSystem::run() {
    std::cout << "Optimized Avatar System running. Press keys to change expressions, ESC to exit." << std::endl;
    
    bool running = true;
    SDL_Event event;
    
    // Initial render
    render();
    
    while (running) {
        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (shouldProcessKey(event.key.keysym.sym)) {
                    handleKeyPress(event.key.keysym.sym);
                    lastKey = event.key.keysym.sym;
                    lastKeyTime = std::chrono::steady_clock::now();
                }
            }
        }
        
        // Update animations
        updateAnimations();
        
        // Render only when needed
        if (animationSystem->isAnimationPlaying() || animationSystem->shouldBlink()) {
            render();
        }
        
        // Frame rate limiting
        rendererManager->waitForNextFrame();
    }
}

void OptimizedAvatarSystem::handleKeyPress(SDL_Keycode key) {
    // Handle horizontal flip
    if (key == SDLK_g) {
        isFlipped = !isFlipped;
        std::cout << "Toggled horizontal flip: " << (isFlipped ? "ON" : "OFF") << std::endl;
        render();
        return;
    }
    
    // Check if key is mapped
    auto it = keyMappings.find(key);
    if (it == keyMappings.end()) {
        return;
    }
    
    const PoseExpression& mapping = it->second;
    int oldPose = currentPose;
    
    if (mapping.pose == currentPose) {
        // Same pose - toggle expression
        if (key == lastKey) {
            currentExpression = (currentExpression == 1) ? mapping.expression : 1;
            std::cout << "Toggled to Pose " << currentPose << ", Expression " << currentExpression << std::endl;
        } else {
            currentExpression = mapping.expression;
            std::cout << "Changed to Pose " << currentPose << ", Expression " << currentExpression << std::endl;
        }
    } else {
        // New pose - play transition animation
        int newExpression = mapping.expression;
        std::cout << "Transitioning to Pose " << mapping.pose << ", Expression " << newExpression << std::endl;

        animationSystem->playPoseTransition(oldPose, mapping.pose);
        currentPose = mapping.pose;
        currentExpression = newExpression;
    }
    
    render();
}

bool OptimizedAvatarSystem::shouldProcessKey(SDL_Keycode key) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastKeyTime);
    return elapsed >= KEY_COOLDOWN;
}

void OptimizedAvatarSystem::updateAnimations() {
    animationSystem->update();
}

void OptimizedAvatarSystem::render() {
    renderControlPanel();
    renderOutputWindow();
    rendererManager->present();
}

void OptimizedAvatarSystem::renderControlPanel() {
    auto* controlTarget = rendererManager->getControlTarget();
    
    // Clear control panel
    rendererManager->clearTarget(controlTarget, {0, 0, 0, 255});
    
    // Get current texture
    SDL_Texture* currentTex = getCurrentTexture();
    if (currentTex) {
        // Render in left third of control panel
        int texWidth, texHeight;
        SDL_QueryTexture(currentTex, nullptr, nullptr, &texWidth, &texHeight);
        
        SDL_Rect destRect;
        destRect.w = texWidth;
        destRect.h = texHeight;
        destRect.x = (WINDOW_WIDTH / 3 - texWidth) / 2;
        destRect.y = (WINDOW_HEIGHT - texHeight) / 2;
        
        if (isFlipped) {
            SDL_RenderCopyEx(controlTarget->renderer, currentTex, nullptr, &destRect, 0, nullptr, SDL_FLIP_HORIZONTAL);
        } else {
            SDL_RenderCopy(controlTarget->renderer, currentTex, nullptr, &destRect);
        }
    }
    
    // Render UI elements
    renderUIElements();
}

void OptimizedAvatarSystem::renderOutputWindow() {
    auto* outputTarget = rendererManager->getOutputTarget();
    
    // Clear output window
    rendererManager->clearTarget(outputTarget, BACKGROUND_COLOR);
    
    // Get current texture or animation frame
    SDL_Texture* currentTex = getCurrentTexture();
    if (currentTex) {
        rendererManager->renderTextureToTarget(outputTarget, currentTex, isFlipped);
    }
}

void OptimizedAvatarSystem::renderUIElements() {
    if (!font) return;
    
    auto* controlTarget = rendererManager->getControlTarget();
    SDL_Color white = {255, 255, 255, 255};
    
    // Status text
    std::string poseName = poses.count(currentPose) ? poses[currentPose] : std::to_string(currentPose);
    std::string statusText = "Current: Pose " + std::to_string(currentPose) +
                           " (" + poseName + "), Exp " + std::to_string(getEffectiveExpression()) +
                           ", Flip: " + (isFlipped ? "ON" : "OFF");
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, statusText.c_str(), white);
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(controlTarget->renderer, textSurface);
        if (textTexture) {
            SDL_Rect textRect = {20, 20, textSurface->w, textSurface->h};
            SDL_RenderCopy(controlTarget->renderer, textTexture, nullptr, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    
    // Controls guide
    const char* controls[] = {
        "Controls:",
        "Q,A,Z: Pose 1",
        "W,S,X: Pose 3", 
        "E,D,C: Pose 4",
        "R,F,V: Pose 6",
        "G: Flip",
        "ESC: Exit"
    };
    
    int yPos = 70;
    for (const char* control : controls) {
        textSurface = TTF_RenderText_Solid(font, control, white);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(controlTarget->renderer, textSurface);
            if (textTexture) {
                SDL_Rect textRect = {WINDOW_WIDTH / 2, yPos, textSurface->w, textSurface->h};
                SDL_RenderCopy(controlTarget->renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        yPos += 25;
    }
}

SDL_Texture* OptimizedAvatarSystem::getCurrentTexture() {
    int effectiveExpression = getEffectiveExpression();
    
    // Check if we have an animation frame
    SDL_Surface* animFrame = animationSystem->getCurrentFrame();
    if (animFrame) {
        return SDL_CreateTextureFromSurface(rendererManager->getControlTarget()->renderer, animFrame);
    }
    
    // Get regular texture
    return resourceManager->getImageTexture(currentPose, effectiveExpression, 
                                          rendererManager->getControlTarget()->renderer);
}

int OptimizedAvatarSystem::getEffectiveExpression() {
    return animationSystem->getCurrentExpression(currentExpression);
}

void OptimizedAvatarSystem::shutdown() {
    std::cout << "Shutting down Optimized Avatar System..." << std::endl;
    
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    
    // Smart pointers will automatically clean up
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    std::cout << "Shutdown complete." << std::endl;
}