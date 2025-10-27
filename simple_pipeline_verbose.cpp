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
#include "utils/metrics.cpp"

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <transform_name> <chunk_size> <image_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " DCT 8 Datasets/KodakImages/1.png" << std::endl;
        std::cerr << "\nAvailable transforms: DCT, SP, HAAR" << std::endl;
        return 1;
    }
    
    // Get transform name and convert to uppercase
    std::string transformName = argv[1];
    std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper);
    
    // Get chunk size
    int chunkSize = std::stoi(argv[2]);
    
    // Check if quantization should be applied (only supports chunk size 8)
    bool applyQuantization = (chunkSize == 8);
    if (!applyQuantization) {
        std::cout << "Note: Quantization will be skipped (only supports chunk size 8)" << std::endl;
    }
    
    // Get image path
    std::string imagePath = argv[3];
    
    // ============================================================================
    // 1. READ IMAGE
    // ============================================================================
    std::cout << "\n=== STEP 1: READ IMAGE ===" << std::endl;
    std::cout << "Loading image from: " << imagePath << std::endl;
    Image originalImg(imagePath);
    Image img(originalImg); // Keep a copy for later conversion
    std::cout << "Image loaded successfully!" << std::endl;
    
    // Calculate entropy of original image
    double originalEntropy = originalImg.getEntropy();
    std::cout << "Original image entropy: " << originalEntropy << " bits per pixel" << std::endl;
    
    // ============================================================================
    // 2. CONVERT TO YCBCR
    // ============================================================================
    std::cout << "\n=== STEP 2: CONVERT TO YCBCR ===" << std::endl;
    std::cout << "Converting image to YCbCr..." << std::endl;
    img.convertToYCbCr();
    std::cout << "Conversion to YCbCr complete!" << std::endl;
    
    // ============================================================================
    // 3. TRANSFORM
    // ============================================================================
    std::cout << "\n=== STEP 3: APPLY TRANSFORM ===" << std::endl;
    
    // Create ChunkedImage with specified chunk size
    std::cout << "Creating ChunkedImage with chunk size " << chunkSize << "..." << std::endl;
    ChunkedImage chunkedImg(img, chunkSize);
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
    
    std::cout << "Using transform: " << transformName << std::endl;
    
    // Apply forward transform (encoding)
    std::cout << "\nApplying forward transform (encoding)..." << std::endl;
    ChunkedImage transformedImg = transform->applyTransform(chunkedImg);
    std::cout << "Transform applied!" << std::endl;
    
    // Calculate entropy of transformed coefficients
    Image transformedImg_view(transformedImg);
    double transformedEntropy = transformedImg_view.getEntropy();
    std::cout << "Transformed coefficients entropy: " << transformedEntropy << " bits per pixel" << std::endl;
    
    // ============================================================================
    // 4. QUANTIZE
    // ============================================================================
    ChunkedImage quantizedImg = transformedImg;
    
    if (applyQuantization) {
        std::cout << "\n=== STEP 4: QUANTIZE ===" << std::endl;
        std::cout << "Applying quantization..." << std::endl;
        quantizedImg = transform->applyQuantization(transformedImg);
        std::cout << "Quantization complete!" << std::endl;
        
        // Calculate entropy of quantized coefficients
        Image quantizedImg_view(quantizedImg);
        double quantizedEntropy = quantizedImg_view.getEntropy();
        std::cout << "Quantized coefficients entropy: " << quantizedEntropy << " bits per pixel" << std::endl;
    } else {
        std::cout << "\n=== STEP 4: QUANTIZE (SKIPPED - chunk size not 8) ===" << std::endl;
        quantizedImg = transformedImg; // Use transformed image without quantization
    }
    
    // ============================================================================
    // 5. ENTROPY ENCODING (COMMENTED OUT - BROKEN)
    // ============================================================================
    std::cout << "\n=== STEP 5: ENTROPY ENCODING (SKIPPED - BROKEN) ===" << std::endl;
    
    EntropyEncoded entropyEncoded;
    if (transformName == "DCT") {
        std::cout << "Applying entropy encoding (DCT only)..." << std::endl;
        entropyEncoded = EntropyEncodeDCT(quantizedImg);
        std::cout << "Entropy encoding complete!" << std::endl;
        
        std::cout << "Applying entropy decoding..." << std::endl;
        EntropyDecodeDCT(quantizedImg, entropyEncoded);
        std::cout << "Entropy decoding complete!" << std::endl;
    }
   
    
    // ============================================================================
    // 6. REVERSE ALL STEPS BACK
    // ============================================================================
    std::cout << "\n=== STEP 6: DECODING (REVERSE ALL STEPS) ===" << std::endl;
    
    // 6a. Inverse quantize
    ChunkedImage dequantizedImg = quantizedImg;
    if (applyQuantization) {
        std::cout << "\n6a. Applying inverse quantization..." << std::endl;
        dequantizedImg = transform->applyInverseQuantization(quantizedImg);
        std::cout << "Inverse quantization complete!" << std::endl;
    } else {
        std::cout << "\n6a. Inverse quantization skipped (chunk size not 8)" << std::endl;
        dequantizedImg = quantizedImg; // Use quantized image without inverse quantization
    }
    
    // 6b. Apply inverse transform
    std::cout << "\n6b. Applying inverse transform (decoding)..." << std::endl;
    ChunkedImage decodedImg = transform->applyInverseTransform(dequantizedImg);
    std::cout << "Inverse transform complete!" << std::endl;
    
    // 6c. Convert back to Image
    std::cout << "\n6c. Converting ChunkedImage to Image..." << std::endl;
    Image resultImg(decodedImg);
    std::cout << "Conversion complete!" << std::endl;
    
    // 6d. Convert back to RGB
    std::cout << "\n6d. Converting back to RGB..." << std::endl;
    resultImg.convertToRGB();
    std::cout << "Conversion to RGB complete!" << std::endl;
    
    // ============================================================================
    // 7. METRICS ON DECODED VS ORIGINAL
    // ============================================================================
    std::cout << "\n=== STEP 7: METRICS ===" << std::endl;
    
    // Calculate MSE
    double mse = metrics::MSE(originalImg, resultImg);
    std::cout << "MSE (Mean Squared Error): " << mse << std::endl;
    
    // Calculate PSNR
    double psnr = metrics::PSNR(originalImg, resultImg);
    std::cout << "PSNR (Peak Signal-to-Noise Ratio): " << psnr << " dB" << std::endl;
    
    // Per-channel MSE
    double mseChannels[3];
    metrics::MSEChannels(originalImg, resultImg, mseChannels);
    std::cout << "Per-channel MSE:" << std::endl;
    std::cout << "  Red channel: " << mseChannels[0] << std::endl;
    std::cout << "  Green channel: " << mseChannels[1] << std::endl;
    std::cout << "  Blue channel: " << mseChannels[2] << std::endl;
    
    // Entropy summary
    std::cout << "\n=== ENTROPY SUMMARY ===" << std::endl;
    std::cout << "Original entropy: " << originalEntropy << " bits/pixel" << std::endl;
    std::cout << "Transformed entropy: " << transformedEntropy << " bits/pixel" << std::endl;
    if (applyQuantization) {
        Image quantizedImg_view(quantizedImg);
        double quantizedEntropy = quantizedImg_view.getEntropy();
        std::cout << "Quantized entropy: " << quantizedEntropy << " bits/pixel" << std::endl;
    } else {
        std::cout << "Quantized entropy: (skipped)" << std::endl;
    }
    
    // ============================================================================
    // 8. SAVE TO PNG
    // ============================================================================
    std::cout << "\n=== STEP 8: SAVE TO PNG ===" << std::endl;
    std::string outputPath = "savedImages/output_" + transformName + ".png";
    std::cout << "Saving image to: " << outputPath << std::endl;
    if (resultImg.saveAsPNG(outputPath)) {
        std::cout << "Image saved successfully!" << std::endl;
    } else {
        std::cerr << "Failed to save image" << std::endl;
        delete transform;
        return 1;
    }
    
    std::cout << "\n=== PIPELINE COMPLETE ===" << std::endl;
    
    // Cleanup
    delete transform;
    
    return 0;
}
