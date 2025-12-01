#include "optimized_avatar_system.h"
#include <iostream>
#include <string>

// Command line options
struct Options {
    bool help = false;
    std::string modelDir = "model";
};

// Parse command line arguments
Options parseArguments(int argc, char* argv[]) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            std::cout << "ChieModel (Optimized) - 2D Virtual Avatar System\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --help, -h        Show this help message\n"
                      << "  --model-dir <dir>  Specify model directory (default: model)\n\n"
                      << "Keyboard Controls:\n"
                      << "  1-9, 0           Change avatar pose (body position)\n"
                      << "  Q, W, E, R...      Change facial expression\n"
                      << "  G                  Toggle horizontal flip\n"
                      << "  ESC                Exit application\n\n"
                      << std::endl;
            options.help = true;
        } else if (arg == "--model-dir" && i + 1 < argc) {
            options.modelDir = argv[++i];
        }
    }

    return options;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    Options options = parseArguments(argc, argv);
    
    if (options.help) {
        return 0;
    }

    // Create and initialize the optimized avatar system
    OptimizedAvatarSystem avatarSystem;
    
    if (!avatarSystem.initialize(options.modelDir)) {
        std::cerr << "Failed to initialize avatar system" << std::endl;
        return 1;
    }

    // Run the main loop
    avatarSystem.run();

    return 0;
}