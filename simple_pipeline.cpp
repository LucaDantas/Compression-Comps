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
#include "utils/timer.hpp"

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
    
    // Measure transform encode time
    ChunkedImage transformedImg = chunkedImg.createFreshCopyForTransformResult(transform->getTransformSpace());
    double encode_ms = 0.0;
    {
        cscomps::util::ScopedTimer t(encode_ms);
        transformedImg = transform->applyTransform(chunkedImg);
    }
    double transformedEntropy = Image(transformedImg).getEntropy();
    
    ChunkedImage quantizedImg = transformedImg;
    double quantizedEntropy = transformedEntropy;
    double quant_ms = 0.0;
    if (applyQuantization) {
        cscomps::util::ScopedTimer tq(quant_ms);
        quantizedImg = transform->applyQuantization(transformedImg);
        // ScopedTimer writes quant_ms on destruction
        quantizedEntropy = Image(quantizedImg).getEntropy();
    }

    EntropyEncoded entropyEncoded;
    double entropy_enc_ms = 0.0;
    double entropy_dec_ms = 0.0;
    if (transformName == "DCT") {
        {
            cscomps::util::ScopedTimer te(entropy_enc_ms);
            entropyEncoded = EntropyEncodeDCT(quantizedImg);
        }
        {
            cscomps::util::ScopedTimer td(entropy_dec_ms);
            EntropyDecodeDCT(quantizedImg, entropyEncoded);
        }
    }
    
    ChunkedImage dequantizedImg = quantizedImg;
    double dequant_ms = 0.0;
    if (applyQuantization) {
        cscomps::util::ScopedTimer tdq(dequant_ms);
        dequantizedImg = transform->applyInverseQuantization(quantizedImg);
    }
    
    double inverse_ms = 0.0;
    ChunkedImage decodedImg = dequantizedImg.createFreshCopyForTransformResult(TransformSpace::Raw);
    {
        cscomps::util::ScopedTimer tinv(inverse_ms);
        decodedImg = transform->applyInverseTransform(dequantizedImg);
    }
    Image resultImg(decodedImg);
    resultImg.convertToRGB();
    
    // std::string outputPath = "savedImages/output_" + transformName + ".png";
    // resultImg.saveAsPNG(outputPath);
    
    double mse = metrics::MSE(originalImg, resultImg);
    double psnr = metrics::PSNR(originalImg, resultImg);
    
    std::cout << "(" << mse << ", " << psnr << ", " << originalEntropy << ", " << transformedEntropy << ", " << quantizedEntropy << ")" << std::endl;
    std::cout << "Times (ms): encode=" << encode_ms
              << " quant=" << quant_ms
              << " entropy_enc=" << entropy_enc_ms
              << " entropy_dec=" << entropy_dec_ms
              << " dequant=" << dequant_ms
              << " inverse=" << inverse_ms << std::endl;
    
    delete transform;
    
    return 0;
}
