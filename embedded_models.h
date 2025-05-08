#ifndef EMBEDDED_MODELS_H
#define EMBEDDED_MODELS_H

#include <map>
#include <string>
#include <vector>

struct EmbeddedImage {
    const char* name;
    const unsigned char* data;
    unsigned int size;
};

// Forward declaration of the embedded images array
extern const EmbeddedImage embedded_images[];
extern const int embedded_images_count;

// Helper function to extract embedded images to a temporary directory
std::string extractEmbeddedImagesToTemp();

#endif // EMBEDDED_MODELS_H
