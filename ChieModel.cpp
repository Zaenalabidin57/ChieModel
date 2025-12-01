#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>
#include <cmath> // For sin() and M_PI
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "embedded_models.h"


// Animation settings
const int ANIMATION_FRAMES = 12;  // Number of frames for jump animation
const double JUMP_HEIGHT = 20.0;  // Maximum jump height in pixels
const int JUMP_SPEED = 16;       // Animation frame delay in milliseconds

// Blink settings
const int BLINK_DURATION = 150;      // Duration of blink in milliseconds (shorter for quicker blink)
const int BLINK_INTERVAL = 3000;    // Time between blinks in milliseconds
const int BLINK_VARIATION = 1000;   // Random variation in blink interval

// Command line options
struct Options {
    bool help = false;       // Show help message
};

// Parse command line arguments
Options parseArguments(int argc, char* argv[]) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            std::cout << "ChieModel - 2D Virtual Avatar System\n"
                      << "Usage: ChieModel [options]\n\n"
                      << "Options:\n"
                      << "  --help, -h   Show this help message\n\n"
                      << "Keyboard Controls:\n"
                      << "  1-9, 0       Change avatar pose (body position)\n"
                      << "  Q, W, E, R...  Change facial expression\n"
                      << "  G            Toggle horizontal flip\n"
                      << "  ESC          Exit application\n\n"
                      << "Note: Press the same key twice to toggle between expression 1\n"
                      << "      and the mapped expression for that key.\n"
                      << std::endl;
            options.help = true;
        }
    }

    return options;
}

// Show help message
void showHelp(const char* programName) {
    std::cout << "\nChieModel Control System\n";
    std::cout << "===================\n\n";
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help      Show this help message\n\n";
    std::cout << "Keyboard Controls:\n";
    std::cout << "  Q, A, Z: Pose 1 (santai)\n";
    std::cout << "  W, S, X: Pose 3 (satu tangan)\n";
    std::cout << "  E, D, C: Pose 4 (belakang tangan)\n";
    std::cout << "  R, F, V: Pose 6 (wawa)\n";
    std::cout << "  G: Toggle horizontal flip\n\n";
    std::cout << "  Press the same key twice to toggle between expression 1 and the specific expression\n";
    std::cout << "  Press ESC to exit\n";
}

// Configuration
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Key cooldown in milliseconds
const int KEY_COOLDOWN = 100;

// Green background for chroma key
const SDL_Color BACKGROUND_COLOR = {0, 255, 0, 255};

// Poses
std::map<int, std::string> POSES = {
    {1, "santai"},
    {3, "satu tangan"},
    {4, "belakang tangan"},
    {6, "wawa"}
};

// Key mappings (QWERTY keyboard layout)
struct PoseExpression {
    int pose;
    int expression;
    
    PoseExpression(int p, int e) : pose(p), expression(e) {}
};

// Map SDL key codes to pose/expression combinations
std::map<SDL_Keycode, PoseExpression> KEY_MAPPINGS = {
    {SDLK_q, PoseExpression(1, 2)},  // Pose 1, Expression 2
    {SDLK_a, PoseExpression(1, 3)},  // Pose 1, Expression 3
    {SDLK_z, PoseExpression(1, 4)},  // Pose 1, Expression 4
    
    {SDLK_w, PoseExpression(3, 2)},  // Pose 3, Expression 2
    {SDLK_s, PoseExpression(3, 3)},  // Pose 3, Expression 3
    {SDLK_x, PoseExpression(3, 4)},  // Pose 3, Expression 4
    
    {SDLK_e, PoseExpression(4, 2)},  // Pose 4, Expression 2
    {SDLK_d, PoseExpression(4, 3)},  // Pose 4, Expression 3
    {SDLK_c, PoseExpression(4, 4)},  // Pose 4, Expression 4
    
    {SDLK_r, PoseExpression(6, 2)},  // Pose 6, Expression 2
    {SDLK_f, PoseExpression(6, 3)},  // Pose 6, Expression 3
    {SDLK_v, PoseExpression(6, 4)},  // Pose 6, Expression 4
};

// Image key type for the map
struct ImageKey {
    int pose;
    int expression;
    
    bool operator<(const ImageKey& other) const {
        if (pose != other.pose) {
            return pose < other.pose;
        }
        return expression < other.expression;
    }
};

class AvatarSystem {
private:
    // SDL components
    SDL_Window* window = nullptr;        // Control panel window
    SDL_Renderer* renderer = nullptr;    // Control panel renderer
    SDL_Window* outputWindow = nullptr;  // Output preview window
    SDL_Renderer* outputRenderer = nullptr; // Output preview renderer
    SDL_Texture* currentTexture = nullptr;

    // Loaded images
    std::map<ImageKey, SDL_Texture*> images;
    std::map<ImageKey, SDL_Surface*> imageSurfaces; // For camera output

    // Animation frames
    std::map<std::pair<int, int>, std::vector<SDL_Surface*>> animationFrames; // Maps start_pose->end_pose to frames

    // State
    int currentPose = 1;
    int currentExpression = 1;
    bool isFlipped = false;  // Track horizontal flip state
    SDL_Keycode lastKey = 0;
    uint32_t lastKeyTime = 0;

    // Blink state
    bool isBlinking = false;
    uint32_t blinkStartTime = 0;
    uint32_t nextBlinkTime = 0;
    int expressionBeforeBlink = 1;

    // Font for rendering text
    TTF_Font* font = nullptr;
    
    // Helper function to create a fallback image
    SDL_Texture* createFallbackTexture() {
        SDL_Surface* surface = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                 32, 0, 0, 0, 0);
        if (!surface) {
            std::cerr << "Failed to create fallback surface: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Fill with green
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format,
                                                BACKGROUND_COLOR.r,
                                                BACKGROUND_COLOR.g,
                                                BACKGROUND_COLOR.b));

        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        return texture;
    }
    
    // Helper to get image path
    std::string getImagePath(int pose, int expression) {
        static std::string modelDir = "";
        
        // If we haven't set up the model directory yet, extract embedded images
        if (modelDir.empty()) {
            modelDir = extractEmbeddedImagesToTemp();
            if (modelDir.empty()) {
                // Fallback to local directory if extraction failed
                std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
                std::filesystem::path exeDir = exePath.parent_path();
                modelDir = (exeDir / "model").string();
            }
        }
        
        std::string filename = std::to_string(pose) + "-" + std::to_string(expression) + ".png";
        return modelDir + "/" + filename;
    }
    
public:
    AvatarSystem() {}

    ~AvatarSystem() {
        shutdown();
    }
    
    bool init(Options options) {
        // Initialize random seed for blink timing
        srand(static_cast<unsigned int>(time(nullptr)));

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Initialize SDL_image
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }

        // Initialize TTF
        if (TTF_Init() < 0) {
            std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
            return false;
        }

        // Load font
        font = TTF_OpenFont("fonts/FreeMono.otf", 16);
        if (!font) {
            std::cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
            // Continue without font - we'll use plain rectangles for UI
        }

        // Create control window
        window = SDL_CreateWindow("ChieModel Control",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               WINDOW_WIDTH, WINDOW_HEIGHT,
                               SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create renderer for control window
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create output preview window
        outputWindow = SDL_CreateWindow("ChieModel Output",
                                     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                     WINDOW_WIDTH, WINDOW_HEIGHT,
                                     SDL_WINDOW_SHOWN);
        if (!outputWindow) {
            std::cerr << "Output window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create output renderer
        outputRenderer = SDL_CreateRenderer(outputWindow, -1, SDL_RENDERER_ACCELERATED);
        if (!outputRenderer) {
            std::cerr << "Output renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(outputWindow);
            outputWindow = nullptr;
            return false;
        }

        // Create model directory if it doesn't exist
        std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
        std::filesystem::path exeDir = exePath.parent_path();
        std::filesystem::path modelDir = exeDir / "model";

        if (!std::filesystem::exists(modelDir)) {
            std::filesystem::create_directory(modelDir);
            std::cout << "Created directory: " << modelDir.string() << std::endl;
            std::cout << "Please place your ChieModel images in this directory" << std::endl;
        }

        // Load all ChieModel images
        loadImages();

        // Generate animation frames for pose transitions
        generateAnimationFrames();

        return true;
    }
    
    void loadImages() {
        int loadedCount = 0;
        
        // Load all possible poses and expressions
        for (const auto& poseEntry : POSES) {
            int pose = poseEntry.first;
            
            for (int expression = 1; expression <= 4; expression++) {
                std::string path = getImagePath(pose, expression);
                
                if (std::filesystem::exists(path)) {
                    // Load image
                    SDL_Surface* surface = IMG_Load(path.c_str());
                    if (!surface) {
                        std::cerr << "Failed to load image: " << path << "\nError: " << IMG_GetError() << std::endl;
                        continue;
                    }
                    
                    // Create texture from surface
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (!texture) {
                        std::cerr << "Failed to create texture from " << path << std::endl;
                        SDL_FreeSurface(surface);
                        continue;
                    }
                    
                    // Store the image
                    ImageKey key = {pose, expression};
                    images[key] = texture;
                    imageSurfaces[key] = surface; // Keep surface for camera output
                    
                    std::cout << "Loaded image: " << path << std::endl;
                    loadedCount++;
                }
            }
        }
        
        std::cout << "Loaded " << loadedCount << " images" << std::endl;
        
        // Create a fallback image if none were found
        if (loadedCount == 0) {
            std::cerr << "Warning: No ChieModel images were loaded!" << std::endl;
            ImageKey defaultKey = {1, 1};
            images[defaultKey] = createFallbackTexture();

            // Create fallback surface for output
            SDL_Surface* surface = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                     32, 0, 0, 0, 0);
            SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format,
                                                  BACKGROUND_COLOR.r,
                                                  BACKGROUND_COLOR.g,
                                                  BACKGROUND_COLOR.b));
            imageSurfaces[defaultKey] = surface;
        }
    }
    
    SDL_Texture* getCurrentTexture() {
        // Determine which expression to show
        int expressionToShow = currentExpression;
        if (isBlinking) {
            expressionToShow = 3; // Show expression 3 (blink) when blinking
        }

        ImageKey key = {currentPose, expressionToShow};

        // Try exact match
        if (images.find(key) != images.end()) {
            return images[key];
        }

        // Fall back to default pose with this expression
        key.pose = 1;
        if (images.find(key) != images.end()) {
            return images[key];
        }

        // Last resort: default image
        key.expression = 1;
        if (images.find(key) != images.end()) {
            return images[key];
        }

        return nullptr;
    }
    
    SDL_Surface* getCurrentSurface() {
        // Determine which expression to show
        int expressionToShow = currentExpression;
        if (isBlinking) {
            expressionToShow = 3; // Show expression 3 (blink) when blinking
        }

        ImageKey key = {currentPose, expressionToShow};

        // Try exact match
        if (imageSurfaces.find(key) != imageSurfaces.end()) {
            return imageSurfaces[key];
        }

        // Fall back to default pose with this expression
        key.pose = 1;
        if (imageSurfaces.find(key) != imageSurfaces.end()) {
            return imageSurfaces[key];
        }

        // Last resort: default image
        key.expression = 1;
        if (imageSurfaces.find(key) != imageSurfaces.end()) {
            return imageSurfaces[key];
        }

        return nullptr;
    }
    
    void renderText(const std::string& text, int x, int y, SDL_Color color) {
        if (!font) return;
        
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    
    void drawControlPanel() {
        // Clear the renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);
        
        // Get current texture
        SDL_Texture* currentTex = getCurrentTexture();
        if (currentTex) {
            // Get texture dimensions
            int width, height;
            SDL_QueryTexture(currentTex, nullptr, nullptr, &width, &height);

            // Calculate position to center the image in the left part of the screen
            SDL_Rect destRect;
            destRect.w = width;
            destRect.h = height;
            destRect.x = (WINDOW_WIDTH / 3 - width) / 2;
            destRect.y = (WINDOW_HEIGHT - height) / 2;

            // Render the ChieModel with optional horizontal flip
            if (isFlipped) {
                SDL_RenderCopyEx(renderer, currentTex, nullptr, &destRect, 0, nullptr, SDL_FLIP_HORIZONTAL);
            } else {
                SDL_RenderCopy(renderer, currentTex, nullptr, &destRect);
            }
        }
        
        // Draw status text
        SDL_Color white = {255, 255, 255, 255};
        std::string poseName = POSES.count(currentPose) ? POSES[currentPose] : std::to_string(currentPose);
        std::string statusText = "Current Pose: " + std::to_string(currentPose) +
                               " (" + poseName + "), Expression: " +
                               std::to_string(currentExpression) +
                               ", Flip: " + (isFlipped ? "ON" : "OFF");
        renderText(statusText, 20, 20, white);
        
        // Draw controls guide
        int yPos = 70;
        renderText("Keyboard Controls:", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("Q, A, Z: Pose 1 (santai)", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("W, S, X: Pose 3 (satu tangan)", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("E, D, C: Pose 4 (belakang tangan)", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("R, F, V: Pose 6 (wawa)", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("G: Toggle horizontal flip", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("Press same key twice to reset to expression 1", WINDOW_WIDTH / 2, yPos, white);
        yPos += 30;
        renderText("Press ESC to exit", WINDOW_WIDTH / 2, yPos, white);
        
        // Update the screen
        SDL_RenderPresent(renderer);
    }
    
    void updateOutputWindow(SDL_Surface* surface) {
        if (!outputWindow || !outputRenderer || !surface) {
            return;
        }

        // Clear the output renderer
        SDL_SetRenderDrawColor(outputRenderer,
                             BACKGROUND_COLOR.r,
                             BACKGROUND_COLOR.g,
                             BACKGROUND_COLOR.b,
                             BACKGROUND_COLOR.a);
        SDL_RenderClear(outputRenderer);

        // Create texture from surface
        SDL_Texture* outputTexture = SDL_CreateTextureFromSurface(outputRenderer, surface);
        if (!outputTexture) {
            std::cerr << "Failed to create output texture" << std::endl;
            return;
        }

        // Get texture dimensions
        int width, height;
        SDL_QueryTexture(outputTexture, nullptr, nullptr, &width, &height);

        // Center the texture in the output window
        SDL_Rect destRect;
        destRect.w = width;
        destRect.h = height;
        destRect.x = (WINDOW_WIDTH - width) / 2;
        destRect.y = (WINDOW_HEIGHT - height) / 2;

        // Render the texture with optional horizontal flip
        if (isFlipped) {
            SDL_RenderCopyEx(outputRenderer, outputTexture, nullptr, &destRect, 0, nullptr, SDL_FLIP_HORIZONTAL);
        } else {
            SDL_RenderCopy(outputRenderer, outputTexture, nullptr, &destRect);
        }
        SDL_RenderPresent(outputRenderer);

        // Clean up
        SDL_DestroyTexture(outputTexture);
    }
    
    void handleKeyPress(SDL_Keycode key) {
        // Handle 'g' key for horizontal flip
        if (key == SDLK_g) {
            isFlipped = !isFlipped;
            std::cout << "Toggled horizontal flip: " << (isFlipped ? "ON" : "OFF") << std::endl;

            // Update output window
            SDL_Surface* surface = getCurrentSurface();
            if (surface) {
                updateOutputWindow(surface);
            }
            return;
        }

        // Check if key is mapped
        if (KEY_MAPPINGS.find(key) == KEY_MAPPINGS.end()) {
            return;
        }
        
        const PoseExpression& mapping = KEY_MAPPINGS.at(key);
        int newPose = mapping.pose;
        int newExpression = mapping.expression;
        int oldPose = currentPose;
        
        // If pressing a key for the current pose
        if (newPose == currentPose) {
            // If the same key is pressed again, toggle between expression 1 and the mapped expression
            if (key == lastKey) {
                if (currentExpression == 1) {
                    // If currently at expression 1, switch to the mapped expression
                    currentExpression = newExpression;
                    std::cout << "Toggled to Pose " << newPose << ", Expression " << newExpression << std::endl;
                } else {
                    // If at any other expression, switch to expression 1
                    currentExpression = 1;
                    std::cout << "Toggled to Pose " << newPose << ", Expression 1" << std::endl;
                }
            } else {
                // First press of a different key for this pose, set to the mapped expression
                currentExpression = newExpression;
                std::cout << "Changed to Pose " << newPose << ", Expression " << newExpression << std::endl;
            }
        } else {
            // Changing to a new pose - perform animation
            std::cout << "Changed to Pose " << newPose << ", Expression " << newExpression << std::endl;
            
            // Save current expression for reference
            //int oldExpression = currentExpression;
            
            // Update the state
            currentPose = newPose;
            currentExpression = newExpression;
            
            // Play animation for pose transition
            playPoseAnimation(oldPose, newPose);
        }
        
        lastKey = key;

        // Update output window
        SDL_Surface* surface = getCurrentSurface();
        if (surface) {
            updateOutputWindow(surface);
        }
    }
    
    void generateAnimationFrames() {
        std::cout << "Generating animation frames for pose transitions..." << std::endl;
        
        // Get all available poses
        std::vector<int> availablePoses;
        for (const auto& entry : POSES) {
            availablePoses.push_back(entry.first);
        }
        
        // For each pair of poses, generate transition animation frames
        for (size_t i = 0; i < availablePoses.size(); i++) {
            for (size_t j = 0; j < availablePoses.size(); j++) {
                // Skip same pose transitions
                if (i == j) continue;
                
                int startPose = availablePoses[i];
                int endPose = availablePoses[j];
                
                // Only generate if we have the necessary start and end images
                ImageKey startKey = {startPose, 1}; // Use default expression
                ImageKey endKey = {endPose, 1};     // Use default expression
                
                if (imageSurfaces.find(startKey) == imageSurfaces.end() ||
                    imageSurfaces.find(endKey) == imageSurfaces.end()) {
                    continue;
                }
                
                // Get the source and destination surfaces
                SDL_Surface* startSurface = imageSurfaces[startKey];
                SDL_Surface* endSurface = imageSurfaces[endKey];
                
                // Create animation frames
                std::vector<SDL_Surface*> frames;
                
                for (int frame = 0; frame < ANIMATION_FRAMES; frame++) {
                    // Calculate progress (0.0 to 1.0)
                    double progress = static_cast<double>(frame) / (ANIMATION_FRAMES - 1);
                    
                    // Calculate jump height using sine wave for smooth arc
                    double jumpOffset = JUMP_HEIGHT * sin(progress * M_PI);

                    // Create a new surface with green background
                    SDL_Surface* animFrame = SDL_CreateRGBSurface(0,
                                                                WINDOW_WIDTH,
                                                                WINDOW_HEIGHT,
                                                                32, 0, 0, 0, 0);
                    
                    // Fill with green
                    SDL_FillRect(animFrame, nullptr, SDL_MapRGB(animFrame->format, 
                                                            BACKGROUND_COLOR.r, 
                                                            BACKGROUND_COLOR.g, 
                                                            BACKGROUND_COLOR.b));
                    
                    // Decide which image to use based on progress
                    SDL_Surface* sourceSurface = (progress < 0.5) ? startSurface : endSurface;
                    
                    // Center the image with jump offset
                    SDL_Rect srcRect;
                    srcRect.x = 0;
                    srcRect.y = 0;
                    srcRect.w = sourceSurface->w;
                    srcRect.h = sourceSurface->h;
                    
                    SDL_Rect destRect;
                    destRect.w = sourceSurface->w;
                    destRect.h = sourceSurface->h;
                    destRect.x = (WINDOW_WIDTH - sourceSurface->w) / 2;
                    destRect.y = (WINDOW_HEIGHT - sourceSurface->h) / 2 - static_cast<int>(jumpOffset);
                    
                    // Blit the image onto the frame
                    SDL_BlitSurface(sourceSurface, &srcRect, animFrame, &destRect);
                    
                    // Add to frames
                    frames.push_back(animFrame);
                }
                
                // Store in animation frames map
                std::pair<int, int> transition(startPose, endPose);
                animationFrames[transition] = frames;
            }
        }
        
        std::cout << "Generated " << animationFrames.size() << " pose transition animations" << std::endl;
    }
    
    void playPoseAnimation(int startPose, int endPose) {
        // Check if we have an animation for this transition
        std::pair<int, int> transition(startPose, endPose);
        if (animationFrames.find(transition) == animationFrames.end()) {
            return; // No animation available
        }
        
        const auto& frames = animationFrames[transition];
        
        // Play each frame
        for (const auto& frame : frames) {
            // Update output window
            updateOutputWindow(frame);
            
            // Process events to keep UI responsive
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    return; // Exit animation if window is closed
                }
            }
            
            // Small delay between frames
            SDL_Delay(JUMP_SPEED);
        }
    }
    
    void run() {
        std::cout << "ChieModel system ready. Press keys to change expressions, ESC to exit." << std::endl;

        // Initialize blink timing
        uint32_t currentTime = SDL_GetTicks();
        nextBlinkTime = currentTime + BLINK_INTERVAL + (rand() % BLINK_VARIATION);

        // Initial update
        drawControlPanel();

        // Update output window with initial image
        SDL_Surface* initialSurface = getCurrentSurface();
        if (initialSurface) {
            updateOutputWindow(initialSurface);
        }

        bool running = true;
        SDL_Event event;

        while (running) {
            currentTime = SDL_GetTicks();

            // Update blink state
            if (!isBlinking && currentTime >= nextBlinkTime) {
                // Start blinking
                isBlinking = true;
                blinkStartTime = currentTime;
                expressionBeforeBlink = currentExpression;
            } else if (isBlinking && currentTime >= blinkStartTime + BLINK_DURATION) {
                // Stop blinking
                isBlinking = false;
                // Schedule next blink
                nextBlinkTime = currentTime + BLINK_INTERVAL + (rand() % BLINK_VARIATION);
            }

            // Process events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    // Exit on ESC
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else {
                        // Check for key cooldown to prevent multiple triggers
                        if (currentTime - lastKeyTime > KEY_COOLDOWN) {
                            handleKeyPress(event.key.keysym.sym);
                            drawControlPanel();
                            lastKeyTime = currentTime;
                        }
                    }
                }
            }

            // Update output every frame for smooth blinking animation
            SDL_Surface* surface = getCurrentSurface();
            if (surface) {
                updateOutputWindow(surface);
            }

            // Limit frame rate
            SDL_Delay(16); // About 60 FPS
        }
    }
    
    void shutdown() {
        std::cout << "\nExiting ChieModel system..." << std::endl;
        
        // Clean up image textures
        for (auto& pair : images) {
            if (pair.second) {
                SDL_DestroyTexture(pair.second);
            }
        }
        images.clear();
        
        // Clean up image surfaces
        for (auto& pair : imageSurfaces) {
            if (pair.second) {
                SDL_FreeSurface(pair.second);
            }
        }
        imageSurfaces.clear();
        
        // Clean up animation frames
        for (auto& animation : animationFrames) {
            for (auto& frame : animation.second) {
                if (frame) {
                    SDL_FreeSurface(frame);
                }
            }
        }
        animationFrames.clear();
        
        // Free font
        if (font) {
            TTF_CloseFont(font);
            font = nullptr;
        }
        
        // Clean up SDL
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        
        if (outputWindow) {
            SDL_DestroyWindow(outputWindow);
            outputWindow = nullptr;
        }
        
        if (outputRenderer) {
            SDL_DestroyRenderer(outputRenderer);
            outputRenderer = nullptr;
        }
        
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        
        std::cout << "ChieModel system shut down." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Parse command line arguments
    Options options = parseArguments(argc, argv);
    
    if (options.help) {
        showHelp(argv[0]);
        return 0;
    }
    
    // Create and initialize ChieModel system
    AvatarSystem ChieModel;
    
    if (!ChieModel.init(options)) {
        std::cerr << "Failed to initialize ChieModel system!" << std::endl;
        return 1;
    }
    
    // Run the main loop
    try {
        ChieModel.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
