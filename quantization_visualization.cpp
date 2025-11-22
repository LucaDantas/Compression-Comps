#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <map>
#include "utils/image_lib.hpp"
#include "transforms/dct_transform.hpp"
#include "transforms/dft_transform.hpp"
#include "transforms/sp_transform.hpp"
#include "transforms/haar_transform.hpp"

// Function to save a visualized version of a ChunkedImage
void saveVisualizationFrame(const ChunkedImage& chunkedImg, const std::string& filename) {
    // Convert to Image to get pixel access
    Image img(chunkedImg);
    
    // Create visualization image
    Image visImg = img;
    
    // Apply clipping
    for (int row = 0; row < img.getRows(); row++) {
        for (int col = 0; col < img.getColumns(); col++) {
            for (int ch = 0; ch < 3; ch++) {
                int val = img.getPixel(row, col)[ch];
                // Clamp to 0-255
                visImg.getPixel(row, col)[ch] = std::max(0, std::min(255, val));
            }
        }
    }
    
    visImg.saveAsPNG(filename);
}

std::vector<double> getScales(const std::string& transformType) {
    std::vector<double> scales;
    if (transformType == "SP") {
        for (int i = 0; i < 30; i++) scales.push_back(0.5 * std::pow(1.3, i));
    } else if (transformType == "HAAR") {
        for (int i = 7; i < 32; i++) scales.push_back(std::pow(1.3, i - 1));
    } else if (transformType == "DCT") {
        for (int i = 0; i < 30; i++) scales.push_back(std::pow(1.2, i));
    } else if (transformType == "DFT") {
        for (int i = 0; i < 20; i++) scales.push_back(std::pow(1.3, i - 1));
    }
    return scales;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " --path <image_path> [--transform <type>] [--chunksize <size>] [--output <dir>]" << std::endl;
        return 1;
    }
    
    std::string imagePath;
    std::string transformType = "DCT";
    int chunkSize = 8;
    std::string outputDir = "quantization_visualization_output";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--path" && i + 1 < argc) imagePath = argv[++i];
        else if (arg == "--transform" && i + 1 < argc) transformType = argv[++i];
        else if (arg == "--chunksize" && i + 1 < argc) chunkSize = std::stoi(argv[++i]);
        else if (arg == "--output" && i + 1 < argc) outputDir = argv[++i];
    }
    
    if (imagePath.empty()) {
         std::cerr << "Error: --path argument is required." << std::endl;
         return 1;
    }

    // Create output directory
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directories(outputDir);
    }
    
    std::cout << "Loading image: " << imagePath << std::endl;
    Image img(imagePath);
    ChunkedImage chunkedImg(img, chunkSize);
    
    Transform* transform = nullptr;
    if (transformType == "DCT") transform = new DCTTransform();
    else if (transformType == "DFT") transform = new DFTTransform();
    else if (transformType == "SP") transform = new SPTransform();
    else if (transformType == "HAAR") transform = new HaarTransform();
    else {
        std::cerr << "Unknown transform type: " << transformType << std::endl;
        return 1;
    }
    
    std::cout << "Generating quantization visualization for " << transformType << "..." << std::endl;
    
    std::vector<double> scales = getScales(transformType);
    
    // 1. Apply Transform
    ChunkedImage transformed = transform->applyTransform(chunkedImg);
    
    for (size_t i = 0; i < scales.size(); i++) {
        double scale = scales[i];
        
        // 2. Quantize
        ChunkedImage quantized = transform->applyQuantization(transformed, scale);
        
        // 3. Dequantize (Inverse Quantization)
        ChunkedImage dequantized = transform->applyInverseQuantization(quantized, scale);
        
        // 4. Inverse Transform
        ChunkedImage reconstructed = transform->applyInverseTransform(dequantized);
        
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/scale_%02zu_val_%.2f.png", outputDir.c_str(), i, scale);
        saveVisualizationFrame(reconstructed, filename);
        
        std::cout << "Saved scale " << i << " (val=" << scale << ")" << std::endl;
    }
    
    std::cout << "All frames saved to " << outputDir << std::endl;
    
    // Try to create GIF using convert (ImageMagick)
    std::string gifPath = outputDir + "/quantization.gif";
    // Use delay 50 as requested
    std::string command = "convert -delay 50 -loop 0 " + outputDir + "/scale_*.png " + gifPath;
    std::cout << "Attempting to create GIF: " << command << std::endl;
    
    int result = system(command.c_str());
    if (result == 0) {
        std::cout << "GIF created successfully: " << gifPath << std::endl;
    } else {
        std::cout << "Failed to create GIF (ImageMagick 'convert' might not be installed). Frames are available in " << outputDir << std::endl;
    }
    
    delete transform;
    return 0;
}
