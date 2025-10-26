#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"
#include "utils/sp_transform.hpp"
#include "DWT/haar_transform.hpp"
#include "utils/entropy.hpp"
#include "utils/dpcm.hpp"
#include "utils/rle.hpp"

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <transform_name>" << std::endl;
        std::cerr << "Available transforms: DCT, SP, HAAR" << std::endl;
        return 1;
    }
    
    // Get transform name and convert to uppercase
    std::string transformName = argv[1];
    std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper);
    
    // Validate transform name
    if (transformName != "DCT" && transformName != "SP" && transformName != "HAAR") {
        std::cerr << "Error: Invalid transform name '" << argv[1] << "'. Must be DCT, SP, or HAAR." << std::endl;
        return 1;
    }
    
    // Hardcoded file path (user will modify this)
    std::string imagePath = "Datasets/KodakImages/1.png"; // TODO: user will hardcode this
    
    // Load image
    std::cout << "Loading image from: " << imagePath << std::endl;
    Image img(imagePath);
    std::cout << "Image loaded successfully!" << std::endl;
    
    // Convert to YCbCr
    std::cout << "\nConverting image to YCbCr..." << std::endl;
    img.convertToYCbCr();
    std::cout << "Conversion to YCbCr complete!" << std::endl;
    
    // Create ChunkedImage with chunk size 8
    std::cout << "\nCreating ChunkedImage with chunk size 8..." << std::endl;
    ChunkedImage chunkedImg(img, 8);
    std::cout << "ChunkedImage created!" << std::endl;
    
    // Create transform based on command line argument
    Transform* transform = nullptr;
    if (transformName == "DCT") {
        transform = new DCTTransform();
    } else if (transformName == "SP") {
        transform = new SPTransform();
    } else if (transformName == "HAAR") {
        transform = new HaarTransform();
    }
    
    std::cout << "\nUsing transform: " << transformName << std::endl;
    
    // Apply forward transform (encoding)
    std::cout << "\nApplying forward transform (encoding)..." << std::endl;
    ChunkedImage encodedImg = transform->applyTransform(chunkedImg);
    std::cout << "Encoding complete!" << std::endl;
    
    // Apply entropy encoding (only for DCT)
    EntropyEncoded *entropyEncoded = nullptr;
    if (transformName == "DCT") {
        std::cout << "\nApplying entropy encoding (DCT only)..." << std::endl;
        entropyEncoded = EntropyEncodeDCT(encodedImg);
        std::cout << "Entropy encoding complete!" << std::endl;
        
        std::cout << "\nApplying entropy decoding..." << std::endl;
        // Note: entropy decoding modifies the encodedImg in place
        EntropyDecodeDCT(encodedImg, entropyEncoded);
        std::cout << "Entropy decoding complete!" << std::endl;
    }
    
    // Apply inverse transform (decoding)
    std::cout << "\nApplying inverse transform (decoding)..." << std::endl;
    ChunkedImage decodedImg = transform->applyInverseTransform(encodedImg);
    std::cout << "Decoding complete!" << std::endl;
    
    // Convert back to Image
    Image resultImg(decodedImg);
    
    // Convert back to RGB
    std::cout << "\nConverting back to RGB..." << std::endl;
    resultImg.convertToRGB();
    std::cout << "Conversion to RGB complete!" << std::endl;
    
    // Write to PNG
    std::string outputPath = "savedImages/output_" + transformName + ".png";
    std::cout << "\nSaving image to: " << outputPath << std::endl;
    if (resultImg.saveAsPNG(outputPath)) {
        std::cout << "Image saved successfully!" << std::endl;
    } else {
        std::cerr << "Failed to save image" << std::endl;
        delete transform;
        return 1;
    }
    
    // Cleanup
    delete transform;
    
    // Note: EntropyEncoded cleanup would require freeing all the internal pointers
    // For now, we'll skip this for simplicity
    
    return 0;
}
