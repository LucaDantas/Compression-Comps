#include <iostream>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"


// for now this is a simple test of the DCT transform

int main() {
    std::cout << "Starting DCT Transform test..." << std::endl;
    
    // Load image from file
    std::cout << "Loading image..." << std::endl;
    Image img("Datasets/KodakImages/1.png");
    std::cout << "Image loaded successfully!" << std::endl;
    img.printInfo();
    
    // Print some original pixel values (first 8x8 block)
    std::cout << "\nOriginal image values (first 8x8 block, R channel):" << std::endl;
    for (int i = 0; i < 8 && i < img.getRows(); i++) {
        for (int j = 0; j < 8 && j < img.getColumns(); j++) {
            std::cout << img.getPixel(i, j)[0] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Create ChunkedImage from the original image
    std::cout << "\nCreating ChunkedImage..." << std::endl;
    ChunkedImage chunkedImg(img, 8);
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
    
    // Print some encoded values (first chunk, R channel)
    std::cout << "\nEncoded DCT values (first chunk, R channel):" << std::endl;
    const Chunk& firstChunk = encodedResult.getChunkAt(0);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
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
    
    // Print some decoded values (first chunk, R channel)
    std::cout << "\nDecoded image values (first chunk, R channel):" << std::endl;
    const Chunk& decodedChunk = decodedResult.getChunkAt(0);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << decodedChunk[0][i][j] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Compare original vs decoded (first chunk, R channel)
    std::cout << "\nComparison - Original vs Decoded (first chunk, R channel):" << std::endl;
    const Chunk& originalChunk = chunkedImg.getChunkAt(0);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int original = originalChunk[0][i][j];
            int decoded = decodedChunk[0][i][j];
            int diff = original - decoded;
            std::cout << "(" << original << "," << decoded << "," << diff << ")\t";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nDCT Transform test completed successfully!" << std::endl;
    return 0;
}