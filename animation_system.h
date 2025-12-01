#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <SDL2/SDL.h>
#include <chrono>
#include <memory>

class ResourceManager;

class AnimationSystem {
public:
    enum class AnimationType {
        POSE_TRANSITION,
        BLINK,
        IDLE
    };

private:
    ResourceManager* resourceManager;
    
    // Current animation state
    AnimationType currentType;
    int startPose, endPose;
    int currentFrame;
    int totalFrames;
    std::chrono::steady_clock::time_point frameStartTime;
    bool isPlaying;
    bool isLooping;
    
    // Animation timing
    const std::chrono::milliseconds frameDuration{16}; // ~60 FPS
    const int blinkDurationFrames = 9; // 150ms at 60 FPS
    
    // Blink state
    bool isBlinking;
    std::chrono::steady_clock::time_point blinkStartTime;
    std::chrono::steady_clock::time_point nextBlinkTime;
    int expressionBeforeBlink;
    const std::chrono::milliseconds blinkInterval{3000};
    const std::chrono::milliseconds blinkVariation{1000};
    
public:
    AnimationSystem(ResourceManager* rm);
    
    // Animation control
    void playPoseTransition(int fromPose, int toPose);
    void startBlink();
    void stopBlink();
    void update();
    
    // State queries
    bool isAnimationPlaying() const { return isPlaying; }
    bool isInBlinkState() const { return isBlinking; }
    int getCurrentExpression(int baseExpression) const;
    
    // Get current frame for rendering
    SDL_Surface* getCurrentFrame();
    
    // Check if blink should trigger
    bool shouldBlink();
    
    // Reset blink timer
    void resetBlinkTimer();
    
private:
    void updateBlink();
    void updatePoseTransition();
    void setRandomBlinkInterval();
};

#endif // ANIMATION_SYSTEM_H