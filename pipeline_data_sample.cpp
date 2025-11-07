#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"
#include "utils/sp_transform.hpp"
#include "utils/dft_transform.hpp"
#include "DWT/haar_transform.hpp"
#include "utils/entropy.hpp"
#include "utils/dpcm.hpp"
#include "utils/rle.hpp"
#include "utils/metrics.cpp"
#include "utils/timer.hpp"
#include "utils/huffman.hpp"
#include "utils/binary_io.hpp"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <transform_name> <image_path> <quant_scale>" << std::endl;
        std::cerr << "Example: " << argv[0] << " DCT Datasets/SquaredKodak/1.png 2.0 1" << std::endl;
        std::cerr << "\nAvailable transforms: DCT, SP, HAAR, DFT" << std::endl;
        return 1;
    }
    
    std::string transformName = argv[1];
    std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper);
    std::string imagePath = argv[2];
    double scale = std::strtod(argv[3], nullptr);
    
    // ============================================================================
    // ENCODING PIPELINE
    // ============================================================================
    
    // Read PNG
    Image originalImg(imagePath);
    Image img(originalImg);
    
    // Infer chunk size
    int chunkSize;
    if (transformName == "DFT" || transformName == "DCT") {
        chunkSize = 8;
    } else {
        chunkSize = originalImg.getRows();
    }
    
    // Make chunked image
    if (transformName != "DFT") img.convertToYCbCr(); else img.convertToGrayscale();
    ChunkedImage chunkedImg(img, chunkSize);
    
    // Create transform
    Transform* transform = nullptr;
    if (transformName == "DCT") {
        transform = new DCTTransform();
    } else if (transformName == "SP") {
        transform = new SPTransform();
    } else if (transformName == "HAAR") {
        transform = new HaarTransform();
    } else if (transformName == "DFT") {
        transform = new DFTTransform();
        originalImg.convertToGrayscale();
        originalImg.convertToRGBFromGrayscale();
    } else {
        std::cerr << "Error: Unknown transform " << transformName << std::endl;
        return 1;
    }
    
    // Measure encoding time
    cscomps::util::Timer encodeTimer;
    
    // Apply transform
    ChunkedImage transformedImg = chunkedImg.createFreshCopyForTransformResult(transform->getTransformSpace());
    transformedImg = transform->applyTransform(chunkedImg);
    
    // Apply quantization
    ChunkedImage quantizedImg = transformedImg;
    quantizedImg = transform->applyQuantization(transformedImg, scale);
    double quantizedEntropy = Image(quantizedImg).getEntropy();
    
    // Apply entropy coding (returns vector<int> directly, handles serialization)
    std::vector<int> entropyEncoded = EntropyEncode(quantizedImg);
    
    // Pass through Huffman
    HuffmanEncoder huffmanEncoder;
    std::vector<int> huffmanEncoded = huffmanEncoder.getEncoding(entropyEncoded);
    
    // Write to binary temporary file
    std::string tempFile = "/tmp/compressed_temp_" + std::to_string(getpid()) + ".bin";
    writeVectorToFile(huffmanEncoded, tempFile);
    
    // Calculate original image size in memory (RGB, 8 bits per channel)
    std::size_t originalSizeBytes = static_cast<std::size_t>(originalImg.getRows()) * 
                                     originalImg.getColumns() * ((transformName == "DFT") ? 1 : 3);
    
    // Calculate compressed file size
    std::size_t compressedSizeBytes = std::filesystem::file_size(tempFile);
    
    // Calculate compression ratio
    double compressionRatio = static_cast<double>(originalSizeBytes) / static_cast<double>(compressedSizeBytes);
    
    double encodingTime = encodeTimer.elapsed_ms();
    
    // ============================================================================
    // DECODING PIPELINE (reverse order)
    // ============================================================================
    
    cscomps::util::Timer decodeTimer;
    
    // Read from binary file
    std::vector<int> huffmanDecodedData = readVectorFromFile(tempFile);
    
    // Decode Huffman
    HuffmanDecoder huffmanDecoder;
    std::vector<int> decodedEntropyData = huffmanDecoder.decode(huffmanDecodedData);
    
    // Decode entropy (handles deserialization internally)
    // Create a fresh copy of quantizedImg for decoding (EntropyDecode modifies in place)
    ChunkedImage decodedQuantizedImg = quantizedImg.createFreshCopyForTransformResult(quantizedImg.getTransformSpace());
    EntropyDecode(decodedQuantizedImg, decodedEntropyData);
    
    // Apply inverse quantization
    ChunkedImage dequantizedImg = decodedQuantizedImg;
    dequantizedImg = transform->applyInverseQuantization(decodedQuantizedImg, scale);
    
    // Apply inverse transform
    ChunkedImage decodedImg = dequantizedImg.createFreshCopyForTransformResult(TransformSpace::Raw);
    decodedImg = transform->applyInverseTransform(dequantizedImg);
    
    double decodingTime = decodeTimer.elapsed_ms();
    
    // Convert back to Image
    Image resultImg(decodedImg);
    if (transformName == "DFT") {
        resultImg.convertToRGBFromGrayscale();
    } else {
        resultImg.convertToRGB();
    }
    
    // Calculate metrics
    double mse = metrics::MSE(originalImg, resultImg);
    double psnr = metrics::PSNR(originalImg, resultImg);

    resultImg.saveAsPNG("decodedImage.png");
    Image diffImg = imageDiff(originalImg, resultImg);
    diffImg.saveAsPNG("differenceImage.png");
    
    // Delete temporary file
    std::filesystem::remove(tempFile);
    
    // Output exactly in the specified format: (compression ratio, entropy quantized, mse, psnr, encoding time, decoding time)
    std::cout << "(" 
              << compressionRatio << ", "
              << quantizedEntropy << ", "
              << mse << ", "
              << psnr << ", "
              << encodingTime << ", "
              << decodingTime << ")"
              << std::endl;
    
    delete transform;
    
    return 0;
}