#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <map>
#include <string>
#include <memory>
#include <vector>

class ResourceManager {
public:
    struct ImageKey {
        int pose;
        int expression;

        bool operator<(const ImageKey& other) const {
            if (pose != other.pose) return pose < other.pose;
            return expression < other.expression;
        }
    };

private:
    // Single storage for images - surfaces only, convert to texture on demand
    std::map<ImageKey, std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>> images;
    
    // Texture cache with reference counting
    std::map<std::pair<ImageKey, SDL_Renderer*>, SDL_Texture*> textureCache;
    
    // Pre-rendered animation frames for specific transitions only (not all combinations)
    std::map<std::pair<int, int>, std::vector<std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>>> animationFrames;
    
    // Directory containing the model images
    std::string modelDirectory;
    
    // Performance metrics
    mutable int textureCacheHits = 0;
    mutable int textureCacheMisses = 0;

public:
    ResourceManager();
    ~ResourceManager();
    
    // Initialize with model directory
    bool initialize(const std::string& modelDir);
    
    // Get image surface (raw data)
    SDL_Surface* getImageSurface(int pose, int expression) const;
    
    // Get texture for specific renderer (cached)
    SDL_Texture* getImageTexture(int pose, int expression, SDL_Renderer* renderer);
    
    // Generate animation frames on-demand for specific transition
    const std::vector<std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>>& getAnimationFrames(int startPose, int endPose);
    
    // Clear texture cache for specific renderer (call when renderer is destroyed)
    void clearRendererCache(SDL_Renderer* renderer);
    
    // Get performance statistics
    void getCacheStats(int& hits, int& misses) const {
        hits = textureCacheHits;
        misses = textureCacheMisses;
    }
    
    // Preload commonly used images
    void preloadCommonImages();
    
private:
    // Load single image from file
    std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)>& loadImage(int pose, int expression);
    
    // Generate animation frames for specific transition
    void generateAnimationFrames(int startPose, int endPose);
    
    // Helper to create surface deleter
    static void surfaceDeleter(SDL_Surface* surface) {
        if (surface) SDL_FreeSurface(surface);
    }
};

#endif // RESOURCE_MANAGER_H