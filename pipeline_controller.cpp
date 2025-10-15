#include <iostream>
#include <string>
#include <stdexcept>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"


// for now this is a simple test of the DCT transform

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <chunk_size>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 8" << std::endl;
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
    
    std::cout << "Starting DCT Transform test with chunk size: " << chunkSize << std::endl;
    
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
    
    // Create DCT Transform
    std::cout << "\nCreating DCT Transform..." << std::endl;
    DCTTransform dctTransform;
    std::cout << "DCT Transform created successfully!" << std::endl;
    std::cout << "Transform space: " << transformSpaceToString(dctTransform.getTransformSpace()) << std::endl;
    
    // Apply transform (encoding)
    std::cout << "\nApplying DCT transform (encoding)..." << std::endl;
    ChunkedImage encodedResult = dctTransform.applyTransform(chunkedImg);
    std::cout << "DCT transform applied successfully!" << std::endl;
    std::cout << "Encoded result info:" << std::endl;
    encodedResult.printInfo();
    
    // Calculate entropy of encoded image
    std::cout << "\nCalculating entropy of encoded image..." << std::endl;
    Image encodedImg(encodedResult);
    double encodedEntropy = encodedImg.getEntropy();
    std::cout << "Encoded image entropy: " << encodedEntropy << " bits per pixel" << std::endl;
    
    // Print some encoded values (first chunk, R channel)
    std::cout << "\nEncoded DCT values (first chunk, R channel):" << std::endl;
    const Chunk& firstChunk = encodedResult.getChunkAt(0);
    for (int i = 0; i < chunkSize; i++) {
        for (int j = 0; j < chunkSize; j++) {
            std::cout << firstChunk[0][i][j] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Apply inverse transform (decoding)
    std::cout << "\nApplying inverse DCT transform (decoding)..." << std::endl;
    ChunkedImage decodedResult = dctTransform.applyInverseTransform(encodedResult);
    std::cout << "Inverse DCT transform applied successfully!" << std::endl;
    std::cout << "Decoded result info:" << std::endl;
    decodedResult.printInfo();
    
    // Calculate entropy of decoded image
    std::cout << "\nCalculating entropy of decoded image..." << std::endl;
    Image decodedImg(decodedResult);
    double decodedEntropy = decodedImg.getEntropy();
    std::cout << "Decoded image entropy: " << decodedEntropy << " bits per pixel" << std::endl;
    
    // Save decoded image as PNG
    std::cout << "\nSaving decoded image as PNG..." << std::endl;
    if (decodedImg.saveAsPNG("decodedImage.png")) {
        std::cout << "Decoded image saved successfully as decodedImage.png" << std::endl;
    } else {
        std::cout << "Failed to save decoded image as PNG" << std::endl;
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
    
    // Entropy summary
    std::cout << "\n=== ENTROPY SUMMARY ===" << std::endl;
    std::cout << "Original image entropy:  " << originalEntropy << " bits per pixel" << std::endl;
    std::cout << "Encoded image entropy:   " << encodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Decoded image entropy:   " << decodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->enc): " << (encodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (enc->dec):  " << (decodedEntropy - encodedEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->dec): " << (decodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
    
    std::cout << "\nDCT Transform test completed successfully!" << std::endl;
    return 0;
}