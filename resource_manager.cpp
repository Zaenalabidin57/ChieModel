#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <cmath>
#include <SDL2/SDL.h>

ResourceManager::ResourceManager() : textureCacheHits(0), textureCacheMisses(0) {
}

ResourceManager::~ResourceManager() {
    // Clear texture cache
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    textureCache.clear();
}

bool ResourceManager::initialize(const std::string& modelDir) {
    modelDirectory = modelDir;
    
    // Check if directory exists
    if (!std::filesystem::exists(modelDir)) {
        std::cerr << "Model directory does not exist: " << modelDir << std::endl;
        return false;
    }
    
    // Preload commonly used images
    preloadCommonImages();
    
    return true;
}

void ResourceManager::preloadCommonImages() {
    // Preload pose 1 expressions (most commonly used)
    for (int expression = 1; expression <= 4; expression++) {
        loadImage(1, expression);
    }
    std::cout << "Preloaded commonly used images" << std::endl;
}

std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>& ResourceManager::loadImage(int pose, int expression) {
    ImageKey key = {pose, expression};

    // Check if already loaded
    auto it = images.find(key);
    if (it != images.end()) {
        return it->second; // Already loaded
    }

    std::string filename = std::to_string(pose) + "-" + std::to_string(expression) + ".png";
    std::string fullPath = modelDirectory + "/" + filename;

    SDL_Surface* surface = nullptr;

    if (std::filesystem::exists(fullPath)) {
        surface = IMG_Load(fullPath.c_str());
        if (!surface) {
            std::cerr << "Failed to load image: " << fullPath << " Error: " << IMG_GetError() << std::endl;
        }
    }

    // Create unique_ptr with custom deleter
    auto ptr = std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>(surface, surfaceDeleter);

    // Insert and return reference
    auto result = images.insert(std::make_pair(key, std::move(ptr)));
    return result.first->second;
}

SDL_Surface* ResourceManager::getImageSurface(int pose, int expression) const {
    ImageKey key = {pose, expression};
    
    auto it = images.find(key);
    if (it == images.end()) {
        // Try to load on demand (const_cast is safe here)
        const_cast<ResourceManager*>(this)->loadImage(pose, expression);
        it = images.find(key);
        if (it == images.end()) {
            return nullptr;
        }
    }
    
    return it->second.get();
}

SDL_Texture* ResourceManager::getImageTexture(int pose, int expression, SDL_Renderer* renderer) {
    ImageKey key = {pose, expression};
    std::pair<ImageKey, SDL_Renderer*> cacheKey = {key, renderer};
    
    auto it = textureCache.find(cacheKey);
    if (it != textureCache.end()) {
        textureCacheHits++;
        return it->second;
    }
    
    textureCacheMisses++;
    
    // Get surface first
    SDL_Surface* surface = getImageSurface(pose, expression);
    if (!surface) {
        return nullptr;
    }
    
    // Create texture from surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from surface for pose " << pose << " expression " << expression << std::endl;
        return nullptr;
    }
    
    // Cache the texture
    textureCache[cacheKey] = texture;
    return texture;
}

const std::vector<std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>>& ResourceManager::getAnimationFrames(int startPose, int endPose) {
    std::pair<int, int> transition = {startPose, endPose};
    
    auto it = animationFrames.find(transition);
    if (it != animationFrames.end()) {
        return it->second;
    }
    
    // Generate on-demand
    generateAnimationFrames(startPose, endPose);
    return animationFrames[transition];
}

void ResourceManager::generateAnimationFrames(int startPose, int endPose) {
    const int ANIMATION_FRAMES = 8; // Reduced from 12 for better performance
    const double JUMP_HEIGHT = 15.0; // Reduced for smoother animation
    
    // Get start and end surfaces
    SDL_Surface* startSurface = getImageSurface(startPose, 1);
    SDL_Surface* endSurface = getImageSurface(endPose, 1);
    
    if (!startSurface || !endSurface) {
        std::cerr << "Cannot generate animation: missing surfaces for poses " << startPose << " -> " << endPose << std::endl;
        return;
    }
    
    std::vector<std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>> frames;
    
    for (int frame = 0; frame < ANIMATION_FRAMES; frame++) {
        double progress = static_cast<double>(frame) / (ANIMATION_FRAMES - 1);
        double jumpOffset = JUMP_HEIGHT * sin(progress * M_PI);
        
        // Create animation frame surface (smaller than full window)
        int maxWidth = std::max(startSurface->w, endSurface->w);
        int maxHeight = std::max(startSurface->h, endSurface->h);
        
        SDL_Surface* animFrame = SDL_CreateRGBSurface(0, maxWidth, maxHeight + static_cast<int>(JUMP_HEIGHT),
                                                     32, 0, 0, 0, 0);
        
        if (!animFrame) {
            std::cerr << "Failed to create animation frame surface: " << SDL_GetError() << std::endl;
            continue;
        }
        
        // Fill with transparent background (assuming images have their own backgrounds)
        SDL_FillRect(animFrame, nullptr, SDL_MapRGB(animFrame->format, 0, 255, 0));
        
        // Choose which surface to use
        SDL_Surface* sourceSurface = (progress < 0.5) ? startSurface : endSurface;
        
        // Center the image with jump offset
        SDL_Rect destRect;
        destRect.w = sourceSurface->w;
        destRect.h = sourceSurface->h;
        destRect.x = (maxWidth - sourceSurface->w) / 2;
        destRect.y = (maxHeight - sourceSurface->h) / 2 - static_cast<int>(jumpOffset);
        
        SDL_BlitSurface(sourceSurface, nullptr, animFrame, &destRect);
        
        // Store with custom deleter
        frames.push_back(std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>(animFrame, surfaceDeleter));
    }
    
    std::pair<int, int> transition = {startPose, endPose};
    animationFrames[transition] = std::move(frames);
    
    std::cout << "Generated " << ANIMATION_FRAMES << " animation frames for transition " 
              << startPose << " -> " << endPose << std::endl;
}

void ResourceManager::clearRendererCache(SDL_Renderer* renderer) {
    auto it = textureCache.begin();
    while (it != textureCache.end()) {
        if (it->first.second == renderer) {
            SDL_DestroyTexture(it->second);
            it = textureCache.erase(it);
        } else {
            ++it;
        }
    }
}