#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sys/stat.h>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"
#include "utils/sp_transform.hpp"


// for now this is a simple test of the DCT transform

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <chunk_size> [--transform DCT|SP]" << std::endl;
        std::cerr << "Example: " << argv[0] << " 8" << std::endl;
        std::cerr << "Example: " << argv[0] << " 8 --transform SP" << std::endl;
        std::cerr << "Transform options: DCT (default), SP" << std::endl;
        return 1;
    }
    
    int chunkSize;
    try {
        chunkSize = std::stoi(argv[1]);
        if (chunkSize <= 0) {
            throw std::invalid_argument("Chunk size must be positive");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid chunk size. " << e.what() << std::endl;
        return 1;
    }
    
    // Parse transform option
    std::string transformType = "DCT"; // Default
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--transform" && i + 1 < argc) {
            std::string transformArg = argv[i + 1];
            // Convert to uppercase for case-insensitive comparison
            std::transform(transformArg.begin(), transformArg.end(), transformArg.begin(), ::toupper);
            if (transformArg == "DCT" || transformArg == "SP") {
                transformType = transformArg;
                i++; // Skip the next argument since we consumed it
            } else {
                std::cerr << "Error: Invalid transform type '" << argv[i + 1] << "'. Must be DCT or SP." << std::endl;
                return 1;
            }
        } else if (arg == "--transform") {
            std::cerr << "Error: --transform requires a value (DCT or SP)" << std::endl;
            return 1;
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            return 1;
        }
    }
    
    // Create savedImages directory if it doesn't exist
    const std::string saveDir = "savedImages";
    if (!std::filesystem::exists(saveDir)) {
        if (std::filesystem::create_directory(saveDir)) {
            std::cout << "Created directory: " << saveDir << std::endl;
        } else {
            std::cerr << "Warning: Could not create directory " << saveDir << ". Images will be saved in current directory." << std::endl;
        }
    }
    
    std::cout << "Starting " << transformType << " Transform test with chunk size: " << chunkSize << std::endl;
    
    // Load image from file
    std::cout << "Loading image..." << std::endl;
    Image img("Datasets/KodakImages/1.png");
    std::cout << "Image loaded successfully!" << std::endl;
    img.printInfo();
    
    // Print some original pixel values (first chunk block)
    std::cout << "\nOriginal image values (first " << chunkSize << "x" << chunkSize << " block, R channel):" << std::endl;
    for (int i = 0; i < chunkSize && i < img.getRows(); i++) {
        for (int j = 0; j < chunkSize && j < img.getColumns(); j++) {
            std::cout << img.getPixel(i, j)[0] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Calculate entropy of original image
    std::cout << "\nCalculating entropy of original image..." << std::endl;
    double originalEntropy = img.getEntropy();
    std::cout << "Original image entropy: " << originalEntropy << " bits per pixel" << std::endl;
    
    // Create ChunkedImage from the original image
    std::cout << "\nCreating ChunkedImage..." << std::endl;
    ChunkedImage chunkedImg(img, chunkSize);
    std::cout << "ChunkedImage created successfully!" << std::endl;
    chunkedImg.printInfo();
    
    // Create Transform based on selection
    std::cout << "\nCreating " << transformType << " Transform..." << std::endl;
    Transform* transform = nullptr;
    
    if (transformType == "DCT") {
        transform = new DCTTransform();
    } else if (transformType == "SP") {
        transform = new SPTransform();
    } else {
        std::cerr << "Error: Unknown transform type: " << transformType << std::endl;
        return 1;
    }
    
    std::cout << transformType << " Transform created successfully!" << std::endl;
    std::cout << "Transform space: " << transformSpaceToString(transform->getTransformSpace()) << std::endl;
    
    // Apply transform (encoding)
    std::cout << "\nApplying " << transformType << " transform (encoding)..." << std::endl;
    ChunkedImage encodedResult = transform->applyTransform(chunkedImg);
    std::cout << transformType << " transform applied successfully!" << std::endl;
    std::cout << "Encoded result info:" << std::endl;
    encodedResult.printInfo();
    
    // Calculate entropy of encoded image
    std::cout << "\nCalculating entropy of encoded image..." << std::endl;
    Image encodedImg(encodedResult);
    double encodedEntropy = encodedImg.getEntropy();
    std::cout << "Encoded image entropy: " << encodedEntropy << " bits per pixel" << std::endl;
    
    // Print some encoded values (first chunk, R channel)
    std::cout << "\nEncoded " << transformType << " values (first chunk, R channel):" << std::endl;
    const Chunk& firstChunk = encodedResult.getChunkAt(0);
    for (int i = 0; i < chunkSize; i++) {
        for (int j = 0; j < chunkSize; j++) {
            std::cout << firstChunk[0][i][j] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Apply inverse transform (decoding)
    std::cout << "\nApplying inverse " << transformType << " transform (decoding)..." << std::endl;
    ChunkedImage decodedResult = transform->applyInverseTransform(encodedResult);
    std::cout << "Inverse " << transformType << " transform applied successfully!" << std::endl;
    std::cout << "Decoded result info:" << std::endl;
    decodedResult.printInfo();
    
    // Calculate entropy of decoded image
    std::cout << "\nCalculating entropy of decoded image..." << std::endl;
    Image decodedImg(decodedResult);
    double decodedEntropy = decodedImg.getEntropy();
    std::cout << "Decoded image entropy: " << decodedEntropy << " bits per pixel" << std::endl;
    
    // Save decoded image as PNG
    std::cout << "\nSaving decoded image as PNG..." << std::endl;
    std::string decodedPath = saveDir + "/decodedImage.png";
    if (decodedImg.saveAsPNG(decodedPath)) {
        std::cout << "Decoded image saved successfully as " << decodedPath << std::endl;
    } else {
        std::cout << "Failed to save decoded image as PNG" << std::endl;
    }
    
    // Save original image channels as separate black and white images
    std::cout << "Saving original image channels as separate BW images..." << std::endl;
    std::string originalBasePath = saveDir + "/originalImage";
    if (img.saveAllChannelsAsBW(originalBasePath, 1)) {
        std::cout << "Original image channels saved successfully as BW images in " << saveDir << std::endl;
    } else {
        std::cout << "Failed to save some original image channels as BW images" << std::endl;
    }
    
    // Save decoded image channels as separate black and white images
    std::cout << "Saving decoded image channels as separate BW images..." << std::endl;
    std::string decodedBasePath = saveDir + "/decodedImage";
    if (decodedImg.saveAllChannelsAsBW(decodedBasePath, 1)) {
        std::cout << "Decoded image channels saved successfully as BW images in " << saveDir << std::endl;
    } else {
        std::cout << "Failed to save some decoded image channels as BW images" << std::endl;
    }
    
    // Print some decoded values (first chunk, R channel)
    std::cout << "\nDecoded image values (first chunk, R channel):" << std::endl;
    const Chunk& decodedChunk = decodedResult.getChunkAt(0);
    for (int i = 0; i < chunkSize; i++) {
        for (int j = 0; j < chunkSize; j++) {
            std::cout << decodedChunk[0][i][j] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Compare original vs decoded (first chunk, R channel)
    std::cout << "\nComparison - Original vs Decoded (first chunk, R channel):" << std::endl;
    const Chunk& originalChunk = chunkedImg.getChunkAt(0);
    for (int i = 0; i < chunkSize; i++) {
        for (int j = 0; j < chunkSize; j++) {
            int original = originalChunk[0][i][j];
            int decoded = decodedChunk[0][i][j];
            int diff = original - decoded;
            std::cout << "(" << original << "," << decoded << "," << diff << ")\t";
        }
        std::cout << std::endl;
    }
    
    // Compute difference image between original and decoded
    std::cout << "\nComputing difference image between original and decoded..." << std::endl;
    try {
        Image diffImg = imageDiff(img, decodedImg, 10);
        std::cout << "Difference image computed successfully!" << std::endl;
        
        // Save difference image as PNG
        std::cout << "Saving difference image as PNG..." << std::endl;
        std::string diffPath = saveDir + "/differenceImage.png";
        if (diffImg.saveAsPNG(diffPath)) {
            std::cout << "Difference image saved successfully as " << diffPath << std::endl;
        } else {
            std::cout << "Failed to save difference image as PNG" << std::endl;
        }
        
        // Save difference image channels as separate black and white images
        std::cout << "Saving difference image channels as separate BW images..." << std::endl;
        std::string diffBasePath = saveDir + "/differenceImage";
        if (diffImg.saveAllChannelsAsBW(diffBasePath, 1)) {
            std::cout << "Difference image channels saved successfully as BW images in " << saveDir << std::endl;
        } else {
            std::cout << "Failed to save some difference image channels as BW images" << std::endl;
        }
        
        // Calculate entropy of difference image
        std::cout << "\nCalculating entropy of difference image..." << std::endl;
        double diffEntropy = diffImg.getEntropy();
        std::cout << "Difference image entropy: " << diffEntropy << " bits per pixel" << std::endl;
        
        // Print some difference values (first chunk, R channel)
        std::cout << "\nDifference image values (first " << chunkSize << "x" << chunkSize << " block, R channel):" << std::endl;
        for (int i = 0; i < chunkSize && i < diffImg.getRows(); i++) {
            for (int j = 0; j < chunkSize && j < diffImg.getColumns(); j++) {
                std::cout << diffImg.getPixel(i, j)[0] << "\t";
            }
            std::cout << std::endl;
        }
        
        // Analyze pixel differences between original and decoded images
        std::cout << "\n=== PIXEL ANALYSIS ===" << std::endl;
        
        // Initialize max differences for each channel
        int maxDiff[3] = {0, 0, 0};
        int minPixel[3] = {255, 255, 255};
        int maxPixel[3] = {0, 0, 0};
        int outOfBoundsCount = 0;
        
        // Analyze all pixels
        for (int row = 0; row < img.getRows(); row++) {
            for (int col = 0; col < img.getColumns(); col++) {
                const Pixel& origPixel = img.getPixel(row, col);
                const Pixel& decodedPixel = decodedImg.getPixel(row, col);
                
                // Check each channel
                for (int ch = 0; ch < 3; ch++) {
                    int origVal = origPixel[ch];
                    int decodedVal = decodedPixel[ch];
                    int diff = abs(origVal - decodedVal);
                    
                    // Update max difference for this channel
                    if (diff > maxDiff[ch]) {
                        maxDiff[ch] = diff;
                    }
                    
                    // Check bounds for original pixel
                    if (origVal < 0 || origVal > 255) {
                        outOfBoundsCount++;
                    }
                    if (origVal < minPixel[ch]) minPixel[ch] = origVal;
                    if (origVal > maxPixel[ch]) maxPixel[ch] = origVal;
                    
                    // Check bounds for decoded pixel
                    if (decodedVal < 0 || decodedVal > 255) {
                        outOfBoundsCount++;
                    }
                }
            }
        }
        
        // Display results
        std::cout << "Maximum absolute differences per channel:" << std::endl;
        std::cout << "  Red channel:   " << maxDiff[0] << std::endl;
        std::cout << "  Green channel: " << maxDiff[1] << std::endl;
        std::cout << "  Blue channel:  " << maxDiff[2] << std::endl;
        
        std::cout << "\nPixel value ranges (original image):" << std::endl;
        std::cout << "  Red channel:   [" << minPixel[0] << ", " << maxPixel[0] << "]" << std::endl;
        std::cout << "  Green channel: [" << minPixel[1] << ", " << maxPixel[1] << "]" << std::endl;
        std::cout << "  Blue channel:  [" << minPixel[2] << ", " << maxPixel[2] << "]" << std::endl;
        
        // Check decoded image bounds
        int decodedMinPixel[3] = {255, 255, 255};
        int decodedMaxPixel[3] = {0, 0, 0};
        
        for (int row = 0; row < decodedImg.getRows(); row++) {
            for (int col = 0; col < decodedImg.getColumns(); col++) {
                const Pixel& decodedPixel = decodedImg.getPixel(row, col);
                for (int ch = 0; ch < 3; ch++) {
                    int val = decodedPixel[ch];
                    if (val < decodedMinPixel[ch]) decodedMinPixel[ch] = val;
                    if (val > decodedMaxPixel[ch]) decodedMaxPixel[ch] = val;
                }
            }
        }
        
        std::cout << "\nPixel value ranges (decoded image):" << std::endl;
        std::cout << "  Red channel:   [" << decodedMinPixel[0] << ", " << decodedMaxPixel[0] << "]" << std::endl;
        std::cout << "  Green channel: [" << decodedMinPixel[1] << ", " << decodedMaxPixel[1] << "]" << std::endl;
        std::cout << "  Blue channel:  [" << decodedMinPixel[2] << ", " << decodedMaxPixel[2] << "]" << std::endl;
        
        std::cout << "\nOut of bounds pixels (not in [0, 255]): " << outOfBoundsCount << std::endl;
        
        // Calculate average differences
        double totalDiff[3] = {0, 0, 0};
        int totalPixels = img.getRows() * img.getColumns();
        
        for (int row = 0; row < img.getRows(); row++) {
            for (int col = 0; col < img.getColumns(); col++) {
                const Pixel& origPixel = img.getPixel(row, col);
                const Pixel& decodedPixel = decodedImg.getPixel(row, col);
                
                for (int ch = 0; ch < 3; ch++) {
                    totalDiff[ch] += abs(origPixel[ch] - decodedPixel[ch]);
                }
            }
        }
        
        std::cout << "\nAverage absolute differences per channel:" << std::endl;
        std::cout << "  Red channel:   " << (totalDiff[0] / totalPixels) << std::endl;
        std::cout << "  Green channel: " << (totalDiff[1] / totalPixels) << std::endl;
        std::cout << "  Blue channel:  " << (totalDiff[2] / totalPixels) << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error computing difference image: " << e.what() << std::endl;
    }
    
    // Entropy summary
    std::cout << "\n=== ENTROPY SUMMARY ===" << std::endl;
    std::cout << "Original image entropy:  " << originalEntropy << " bits per pixel" << std::endl;
    std::cout << "Encoded image entropy:   " << encodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Decoded image entropy:   " << decodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->enc): " << (encodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (enc->dec):  " << (decodedEntropy - encodedEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->dec): " << (decodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
    
    std::cout << "\n" << transformType << " Transform test completed successfully!" << std::endl;
    
    // Clean up transform object
    delete transform;
    
    return 0;
}