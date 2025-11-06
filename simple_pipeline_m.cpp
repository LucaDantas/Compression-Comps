#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
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


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <transform_name> <image_path> <quant_scale>" << std::endl;
        std::cerr << "Example: " << argv[0] << " DCT Datasets/SquaredKodak/1.png 2.0" << std::endl;
        std::cerr << "\nAvailable transforms: DCT, SP, HAAR, DFT" << std::endl;
        return 1;
    }
    
    std::string transformName = argv[1];
    std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper);
    std::string imagePath = argv[2];
    double scale = std::strtod(argv[3], nullptr);

    Image originalImg(imagePath);
    Image img(originalImg); 

    int chunkSize;
    if (transformName == "DFT" || transformName == "DCT") {
        chunkSize = 8;
    } else {
        chunkSize = originalImg.getRows();
    }
    
    img.convertToYCbCr();
    Transform* transform = nullptr;
    if (transformName == "DCT") {
        transform = new DCTTransform();
    } else if (transformName == "SP") {
        transform = new SPTransform();
    } else if (transformName == "HAAR") {
        transform = new HaarTransform(); 
    } else if (transformName == "DFT") {
        transform = new DFTTransform();
        img.convertToGrayscale();
        originalImg.convertToYCbCr();
        originalImg.convertToGrayscale();
        originalImg.convertToYCbCrFromGrayscale();
        originalImg.convertToRGB();
    } else {
        std::cerr << "Error: Unknown transform " << transformName << std::endl;
        return 1;
    }

    ChunkedImage chunkedImg(img, chunkSize);

    ChunkedImage transformedImg = chunkedImg.createFreshCopyForTransformResult(transform->getTransformSpace());

    cscomps::util::Timer timer;
    transformedImg = transform->applyTransform(chunkedImg);


    ChunkedImage quantizedImg = transformedImg;
    quantizedImg = transform->applyQuantization(transformedImg, scale);
    double quantizedEntropy = Image(quantizedImg).getEntropy();

    //
    // ENTROPY ENCODING AND HUFFMAN SECTION GOES HERE
    //

    
    double encode_ms = timer.elapsed_ms();
    timer.reset();
    
    // apply inverse quantization
    ChunkedImage dequantizedImg = quantizedImg;
    dequantizedImg = transform->applyInverseQuantization(quantizedImg, scale);

    
    ChunkedImage decodedImg = dequantizedImg.createFreshCopyForTransformResult(TransformSpace::Raw);
    decodedImg = transform->applyInverseTransform(dequantizedImg);
    double decode_ms = timer.elapsed_ms();


    Image resultImg(decodedImg);
    if (transformName == "DFT") {
        resultImg.convertToYCbCrFromGrayscale();
    }
    resultImg.convertToRGB();



    
    double mse = metrics::MSE(originalImg, resultImg);
    double psnr = metrics::PSNR(originalImg, resultImg);
    
    // // Calculate Compression Ratio (CR)
    // // Size is estimated from the entropy-encoded data size
    // std::size_t raw_bytes = static_cast<std::size_t>(originalImg.getRows()) * originalImg.getColumns() * 3;
    // std::size_t encoded_bytes = entropyEncoded.encodedData.size();
    // double compressionRatio = static_cast<double>(raw_bytes) / static_cast<double>(encoded_bytes);

    // // Output (exactly in the specified format)
    // // (compression ratio, entropy quantized, mse, psnr, encoding time, decoding time)
    // double total_encoding_time = encode_ms + quant_ms + entropy_enc_ms;
    // double total_decoding_time = dequant_ms + inverse_ms + entropy_dec_ms; // Inverse quantization + Inverse transform + Entropy decoding

    double compressionRatio = 1;
    
    std::cout << "(" 
              << compressionRatio << ", "
              << quantizedEntropy << ", "
              << mse << ", "
              << psnr << ", "
              << encode_ms << ", "
              << decode_ms << ")"
              << std::endl;

    delete transform;
    
    return 0;
}