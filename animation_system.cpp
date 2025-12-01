#include "animation_system.h"
#include "resource_manager.h"
#include <random>
#include <iostream>

AnimationSystem::AnimationSystem(ResourceManager* rm) 
    : resourceManager(rm)
    , currentType(AnimationType::IDLE)
    , startPose(1), endPose(1)
    , currentFrame(0), totalFrames(0)
    , isPlaying(false), isLooping(false)
    , isBlinking(false)
    , expressionBeforeBlink(1) {
    
    resetBlinkTimer();
}

void AnimationSystem::playPoseTransition(int fromPose, int toPose) {
    if (fromPose == toPose) {
        return; // No animation needed
    }
    
    startPose = fromPose;
    endPose = toPose;
    currentFrame = 0;
    currentType = AnimationType::POSE_TRANSITION;
    isPlaying = true;
    isLooping = false;
    
    // Check if animation frames exist for this transition
    const auto& frames = resourceManager->getAnimationFrames(fromPose, toPose);
    totalFrames = frames.size();
    
    if (totalFrames == 0) {
        std::cerr << "No animation frames available for transition " << fromPose << " -> " << toPose << std::endl;
        isPlaying = false;
        currentType = AnimationType::IDLE;
        return;
    }
    
    frameStartTime = std::chrono::steady_clock::now();
    std::cout << "Starting pose transition animation: " << fromPose << " -> " << toPose 
              << " (" << totalFrames << " frames)" << std::endl;
}

void AnimationSystem::startBlink() {
    if (isBlinking) return;
    
    isBlinking = true;
    blinkStartTime = std::chrono::steady_clock::now();
    currentFrame = 0;
    totalFrames = blinkDurationFrames;
    currentType = AnimationType::BLINK;
    isPlaying = true;
    isLooping = false;
    
    std::cout << "Starting blink animation" << std::endl;
}

void AnimationSystem::stopBlink() {
    if (!isBlinking) return;
    
    isBlinking = false;
    currentType = AnimationType::IDLE;
    isPlaying = false;
    resetBlinkTimer();
    
    std::cout << "Stopping blink animation" << std::endl;
}

void AnimationSystem::update() {
    if (!isPlaying) {
        updateBlink();
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - frameStartTime);
    
    if (elapsed >= frameDuration) {
        currentFrame++;
        frameStartTime = now;
        
        switch (currentType) {
            case AnimationType::POSE_TRANSITION:
                updatePoseTransition();
                break;
            case AnimationType::BLINK:
                if (currentFrame >= totalFrames) {
                    stopBlink();
                }
                break;
            case AnimationType::IDLE:
            default:
                break;
        }
    }
    
    // Also check for blink timing
    updateBlink();
}

void AnimationSystem::updatePoseTransition() {
    if (currentFrame >= totalFrames) {
        isPlaying = false;
        currentType = AnimationType::IDLE;
        std::cout << "Pose transition animation completed" << std::endl;
    }
}

void AnimationSystem::updateBlink() {
    if (isBlinking) return;
    
    auto now = std::chrono::steady_clock::now();
    if (now >= nextBlinkTime) {
        startBlink();
    }
}

SDL_Surface* AnimationSystem::getCurrentFrame() {
    if (!isPlaying) {
        return nullptr;
    }
    
    switch (currentType) {
        case AnimationType::POSE_TRANSITION: {
            const auto& frames = resourceManager->getAnimationFrames(startPose, endPose);
            if (currentFrame < frames.size()) {
                return frames[currentFrame].get();
            }
            break;
        }
        case AnimationType::BLINK:
            // Blink doesn't use pre-rendered frames, handled by getCurrentExpression
            break;
        case AnimationType::IDLE:
        default:
            return nullptr;
    }
    
    return nullptr;
}

int AnimationSystem::getCurrentExpression(int baseExpression) const {
    if (isBlinking) {
        // Map blink frames to expression 3 (blink eyes)
        return 3;
    }
    return baseExpression;
}

bool AnimationSystem::shouldBlink() {
    auto now = std::chrono::steady_clock::now();
    return !isBlinking && now >= nextBlinkTime;
}

void AnimationSystem::resetBlinkTimer() {
    auto now = std::chrono::steady_clock::now();
    setRandomBlinkInterval();
    nextBlinkTime = now + blinkInterval + std::chrono::milliseconds(rand() % blinkVariation.count());
}

void AnimationSystem::setRandomBlinkInterval() {
    // Blink interval variation is handled in resetBlinkTimer
}