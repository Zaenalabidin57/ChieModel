#!/bin/bash
# Simple performance patch for ChieModel.cpp
# This script applies the most impactful performance improvements

echo "Applying performance improvements to ChieModel.cpp..."

# Backup original
cp ChieModel.cpp ChieModel.cpp.backup

# Apply texture caching to avoid recreation every frame
sed -i '/class AvatarSystem {/i\
class TextureCache {\
private:\
    SDL_Texture* cachedOutputTexture = nullptr;\
    bool needsUpdate = true;\
\
public:\
    void markNeedsUpdate() { needsUpdate = true; }\
\
    SDL_Texture* getOrCreateOutputTexture(SDL_Renderer* renderer, SDL_Surface* surface) {\
        if (!needsUpdate && cachedOutputTexture) {\
            return cachedOutputTexture;\
        }\
\
        if (cachedOutputTexture) {\
            SDL_DestroyTexture(cachedOutputTexture);\
        }\
\
        cachedOutputTexture = SDL_CreateTextureFromSurface(renderer, surface);\
        needsUpdate = false;\
        return cachedOutputTexture;\
    }\
\
    ~TextureCache() {\
        if (cachedOutputTexture) {\
            SDL_DestroyTexture(cachedOutputTexture);\
        }\
    }\
};\
\
class AvatarSystem {' ChieModel.cpp

# Add texture cache member
sed -i '/TTF_Font\* font = nullptr;/a\
    TextureCache textureCache;' ChieModel.cpp

# Replace updateOutputWindow to use cached texture
sed -i 's/SDL_Texture\* outputTexture = SDL_CreateTextureFromSurface(outputRenderer, surface);/SDL_Texture* outputTexture = textureCache.getOrCreateOutputTexture(outputRenderer, surface);/' ChieModel.cpp

# Remove texture destruction at end of updateOutputWindow
sed -i '/SDL_DestroyTexture(outputTexture);/d' ChieModel.cpp

# Add cache invalidation in handleKeyPress
sed -i '/currentExpression = newExpression;/a\
        textureCache.markNeedsUpdate();' ChieModel.cpp

# Add cache invalidation after pose changes
sed -i '/currentPose = mapping.pose;/a\
        textureCache.markNeedsUpdate();' ChieModel.cpp

# Add cache invalidation in run loop for blinking
sed -i '/if (isBlinking && currentTime >= blinkStartTime + BLINK_DURATION) {/,/a\
        textureCache.markNeedsUpdate();' ChieModel.cpp

# Add texture cache cleanup in shutdown
sed -i '/TTF_CloseFont(font);/a\
    // Texture cache is automatically cleaned up by destructor' ChieModel.cpp

echo "Performance improvements applied!"
echo ""
echo "Key improvements:"
echo "- Texture caching eliminates recreation every frame"
echo "- Cache invalidation only when state changes"
echo "- Simple implementation without complex template issues"
echo ""
echo "Build with: make"