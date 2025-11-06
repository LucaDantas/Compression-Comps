#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <vector>
// Only keep essential headers
#include "utils/entropy.hpp"
#include "utils/dpcm.hpp"
#include "utils/rle.hpp"
#include "utils/metrics.cpp"
#include "utils/timer.hpp"

// NOTE: We need the ChunkedImage structure to pass to EntropyEncode/Decode,
// but we will bypass all the transformation/quantization logic.

int main(int argc, char* argv[]) {
    if (argc < 2) { // Only need image path now
        std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " Datasets/SquaredKodak/1.png" << std::endl;
        return 1;
    }
    
    // std::string transformName = argv[1]; // REMOVED
    // std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper); // REMOVED
    std::string imagePath = argv[1]; // Use argv[1] for the image path now
    // double scale = std::strtod(argv[3], nullptr); // REMOVED
    
    // ============================================================================
    // SETUP AND ISOLATED ENCODING PIPELINE
    // ============================================================================
    
    // Read PNG
    // We only load the image and convert it to the ChunkedImage format
    // that the Entropy functions expect, *bypassing all transforms*.
    Image originalImg(imagePath);
    Image img(originalImg);

    // 1. Prepare data for EntropyEncode (minimal setup to get a ChunkedImage)
    // For a lossless test, we assume EntropyEncode works on the RAW image data 
    // *after* it has been converted to the component structure it expects.
    // Assuming ChunkedImage is the required input structure.
    
    // --- Minimal conversion to match the expected input type for EntropyEncode ---
    // If EntropyEncode expects YCbCr, keep this:
    img.convertToYCbCr(); 
    int chunkSize = originalImg.getRows(); // Use full image as one "chunk"
    ChunkedImage rawChunkedImg(img, chunkSize);
    
    // The input data is now rawChunkedImg.
    ChunkedImage& inputData = rawChunkedImg; 
    
    // ============================================================================
    // ENTROPY ENCODING (ISOLATED)
    // ============================================================================
    
    std::cout << "--- Starting Entropy-Only Test ---" << std::endl;
    cscomps::util::Timer encodeTimer;
    
    // Apply entropy coding (returns vector<int> directly, handles serialization)
    // This is the core encoding step we want to test.
    std::vector<int> entropyEncoded = EntropyEncode(inputData);
    
    double encodingTime = encodeTimer.elapsed_ms();
    std::cout << "Entropy Encoding Time: " << encodingTime << " ms" << std::endl;
    std::cout << "Encoded Data Size: " << entropyEncoded.size() * sizeof(int) << " bytes" << std::endl;
    
    // ============================================================================
    // ENTROPY DECODING (ISOLATED)
    // ============================================================================
    
    cscomps::util::Timer decodeTimer;
    
    // Create a fresh copy of the input data structure for decoding
    // This copy will hold the result of the decoding process.
    ChunkedImage decodedData = inputData.createFreshCopyForTransformResult(inputData.getTransformSpace());
    
    // Decode entropy (handles deserialization internally)
    // This is the core decoding step we want to test.
    EntropyDecode(decodedData, entropyEncoded);
    
    double decodingTime = decodeTimer.elapsed_ms();
    std::cout << "Entropy Decoding Time: " << decodingTime << " ms" << std::endl;

    // ============================================================================
    // VERIFICATION (ISOLATED)
    // ============================================================================
    
    // To verify, we compare the original input (rawChunkedImg) with the decoded output (decodedData)
    std::cout << "\n--- Verification ---" << std::endl;
    
    // Convert back to Image objects for easy comparison (assuming Image has a comparison method or we use metrics)
    // NOTE: This comparison is crucial for a *lossless* entropy test.
    
    // Convert back to Image objects for MSE comparison (if available)
    Image resultImg(decodedData); // Convert decoded ChunkedImage back to Image
    resultImg.convertToRGB();


    // Use a custom comparison function if available, or MSE/PSNR on the non-RGB data.
    // Since originalImg is the raw data loaded, we can use a basic check.
    
    // A simple check on the actual data vectors for verification
    resultImg.saveAsPNG("decodedImage.png");
    Image diffImg = imageDiff(originalImg, resultImg);
    diffImg.saveAsPNG("differenceImage.png");

    // If they don't match, print some simple metrics for debugging
    double mse_raw = metrics::MSE(originalImg, resultImg);
    double psnr_raw = metrics::PSNR(originalImg, resultImg);
    std::cout << "MSE: " << mse_raw << ", PSNR: " << psnr_raw << std::endl;

    // Output the simple execution times (no complex metrics)
    std::cout << "(Encoding Time: " << encodingTime << " ms, Decoding Time: " << decodingTime << " ms)" << std::endl;
    
    return 0;
}