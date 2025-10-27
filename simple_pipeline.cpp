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
    
    bool applyQuantization = (chunkSize == 8);
    std::string imagePath = argv[3];
    
    // Read and process image
    Image originalImg(imagePath);
    Image img(originalImg);
    double originalEntropy = originalImg.getEntropy();
    
    img.convertToYCbCr();
    ChunkedImage chunkedImg(img, chunkSize);
    
    Transform* transform = nullptr;
    if (transformName == "DCT") {
        transform = new DCTTransform();
    } else if (transformName == "SP") {
        transform = new SPTransform();
    } else if (transformName == "HAAR") {
        transform = new HaarTransform();
    }
    
    ChunkedImage transformedImg = transform->applyTransform(chunkedImg);
    double transformedEntropy = Image(transformedImg).getEntropy();
    
    ChunkedImage quantizedImg = transformedImg;
    double quantizedEntropy = transformedEntropy;
    if (applyQuantization) {
        quantizedImg = transform->applyQuantization(transformedImg);
        quantizedEntropy = Image(quantizedImg).getEntropy();
    }
    
    ChunkedImage dequantizedImg = quantizedImg;
    if (applyQuantization) {
        dequantizedImg = transform->applyInverseQuantization(quantizedImg);
    }
    
    ChunkedImage decodedImg = transform->applyInverseTransform(dequantizedImg);
    Image resultImg(decodedImg);
    resultImg.convertToRGB();
    
    std::string outputPath = "savedImages/output_" + transformName + ".png";
    resultImg.saveAsPNG(outputPath);
    
    double mse = metrics::MSE(originalImg, resultImg);
    double psnr = metrics::PSNR(originalImg, resultImg);
    double mseChannels[3];
    metrics::MSEChannels(originalImg, resultImg, mseChannels);
    
    std::cout << "Transform: " << transformName << ", Chunk Size: " << chunkSize << std::endl;
    std::cout << "MSE: " << mse << ", PSNR: " << psnr << " dB" << std::endl;
    std::cout << "MSE (R/G/B): " << mseChannels[0] << "/" << mseChannels[1] << "/" << mseChannels[2] << std::endl;
    std::cout << "Entropy (original/transformed/quantized): " << originalEntropy << "/" << transformedEntropy << "/" << quantizedEntropy << std::endl;
    std::cout << "Output: " << outputPath << std::endl;
    
    delete transform;
    
    return 0;
}
