#!/usr/bin/env python3
import os
import sys
import base64

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <model_dir> <output_cpp_file>")
        sys.exit(1)
    
    model_dir = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.isdir(model_dir):
        print(f"Error: {model_dir} is not a directory")
        sys.exit(1)
    
    # Get all PNG files
    image_files = []
    for file in os.listdir(model_dir):
        if file.endswith(".png"):
            image_files.append(file)
    
    if not image_files:
        print(f"Error: No PNG files found in {model_dir}")
        sys.exit(1)
    
    # Sort to ensure consistent ordering
    image_files.sort()
    
    # Generate the C++ code
    with open(output_file, "w") as f:
        # Write header
        f.write('#include "embedded_models.h"\n\n')
        f.write('#include <cstddef>\n')
        f.write('#include <fstream>\n')
        f.write('#include <iostream>\n')
        f.write('#include <filesystem>\n')
        f.write('#include <sys/stat.h>\n\n')
        
        # Write each image as a byte array
        for image_file in image_files:
            image_path = os.path.join(model_dir, image_file)
            with open(image_path, "rb") as img_file:
                image_data = img_file.read()
            
            # Convert to C array
            var_name = f"image_{image_file.replace('-', '_').replace('.', '_')}"
            f.write(f"// Embedded image: {image_file}\n")
            f.write(f"static const unsigned char {var_name}[] = {{\n")
            
            # Write data as hex values, 16 per line
            for i in range(0, len(image_data), 16):
                chunk = image_data[i:i+16]
                hex_bytes = [f"0x{b:02x}" for b in chunk]
                f.write("    " + ", ".join(hex_bytes) + ",\n")
            
            f.write("}; // End of image data\n\n")
        
        # Create the array of all embedded images
        f.write("// Array of all embedded images\n")
        f.write("const EmbeddedImage embedded_images[] = {\n")
        
        for image_file in image_files:
            var_name = f"image_{image_file.replace('-', '_').replace('.', '_')}"
            f.write(f"    {{ \"{image_file}\", {var_name}, sizeof({var_name}) }},\n")
        
        f.write("}; // End of embedded_images array\n\n")
        
        # Write count of embedded images
        f.write(f"const int embedded_images_count = {len(image_files)};\n\n")
        
        # Add function to extract images to temp directory
        f.write("// Function to extract embedded images to a temporary directory\n")
        f.write("std::string extractEmbeddedImagesToTemp() {\n")
        f.write("    // Create a unique temporary directory\n")
        f.write("    std::string temp_dir;\n")
        f.write("    char template_dir[] = \"/tmp/chiemodel-XXXXXX\";\n")
        f.write("    char* result = mkdtemp(template_dir);\n")
        f.write("    if (result == nullptr) {\n")
        f.write("        std::cerr << \"Failed to create temporary directory for embedded models!\" << std::endl;\n")
        f.write("        return \"\";\n")
        f.write("    }\n\n")
        f.write("    temp_dir = result;\n")
        f.write("    std::cout << \"Extracting embedded models to: \" << temp_dir << std::endl;\n\n")
        
        f.write("    // Extract each image\n")
        f.write("    for (int i = 0; i < embedded_images_count; i++) {\n")
        f.write("        std::string file_path = temp_dir + \"/\" + embedded_images[i].name;\n")
        f.write("        std::ofstream outfile(file_path, std::ios::binary);\n")
        f.write("        if (!outfile) {\n")
        f.write("            std::cerr << \"Failed to write embedded image: \" << embedded_images[i].name << std::endl;\n")
        f.write("            continue;\n")
        f.write("        }\n")
        f.write("        outfile.write(reinterpret_cast<const char*>(embedded_images[i].data), embedded_images[i].size);\n")
        f.write("        outfile.close();\n")
        f.write("        std::cout << \"Extracted: \" << embedded_images[i].name << std::endl;\n")
        f.write("    }\n\n")
        
        f.write("    return temp_dir;\n")
        f.write("}\n")
    
    print(f"Generated C++ code for {len(image_files)} embedded images in {output_file}")

if __name__ == "__main__":
    main()
