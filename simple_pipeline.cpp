#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
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

int main(int argc, char* argv[]) {
    // Check command line arguments (minimum required: transform chunk_size image_path)
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <transform_name> <chunk_size> <image_path> [options]" << std::endl;
        std::cerr << "Example: " << argv[0] << " DCT 8 Datasets/KodakImages/1.png" << std::endl;
        std::cerr << "\nAvailable transforms: DCT, SP, HAAR" << std::endl;
        std::cerr << "Options (SP only): --scale <float> --q-ll <int> --q-hl <int> --q-lh <int> --q-hh <int> --dz <int> --level-gamma <float>" << std::endl;
        return 1;
    }
    
    // Get transform name and convert to uppercase
    std::string transformName = argv[1];
    std::transform(transformName.begin(), transformName.end(), transformName.begin(), ::toupper);
    
    // Get chunk size
    int chunkSize = std::stoi(argv[2]);
    
    bool applyQuantization = true; // enable quantization by default for experiments
    std::string imagePath = argv[3];

    // Parse optional flags starting at argv[4]
    SPTransform::QuantParams qp = SPTransform::MakeQuantParams(1.0f);
    for (int ai = 4; ai < argc; ++ai) {
        std::string a = argv[ai];
        if (a == "--scale" && ai + 1 < argc) {
            qp.scale = std::stof(argv[++ai]);
        } else if (a == "--q-ll" && ai + 1 < argc) {
            qp.q_LL = std::stoi(argv[++ai]);
        } else if (a == "--q-hl" && ai + 1 < argc) {
            qp.q_HL = std::stoi(argv[++ai]);
        } else if (a == "--q-lh" && ai + 1 < argc) {
            qp.q_LH = std::stoi(argv[++ai]);
        } else if (a == "--q-hh" && ai + 1 < argc) {
            qp.q_HH = std::stoi(argv[++ai]);
        } else if (a == "--dz" && ai + 1 < argc) {
            qp.deadzone = std::max(0, std::stoi(argv[++ai]));
        } else if (a == "--level-gamma" && ai + 1 < argc) {
            qp.level_gamma = std::stof(argv[++ai]);
        } else if (a == "--no-quant") {
            applyQuantization = false;
        } else {
            std::cerr << "Unknown option: " << a << std::endl;
        }
    }
    
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
        // construct SP transform with the requested quant params
        transform = new SPTransform(qp);
    } else if (transformName == "HAAR") {
        transform = new HaarTransform();
    } else if (transformName == "DFT") {
        transform = new DFTTransform();
        img.convertToGrayscale();
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
    
    std::cout << "Transform: " << transformName << ", Chunk Size: " << chunkSize << std::endl;
    std::cout << "MSE: " << mse << ", PSNR: " << psnr << " dB" << std::endl;
    std::cout << "MSE (R/G/B): " << mseChannels[0] << "/" << mseChannels[1] << "/" << mseChannels[2] << std::endl;
    std::cout << "Entropy (original/transformed/quantized): " << originalEntropy << "/" << transformedEntropy << "/" << quantizedEntropy << std::endl;
    std::cout << "Output: " << outputPath << std::endl;

    // Estimate bits-per-pixel using entropies via metrics helpers
    try {
        int rows = originalImg.getRows();
        int cols = originalImg.getColumns();
        // raw bytes (RGB 8-bit per channel)
        std::size_t raw_bytes = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * 3;

        // estimate bytes from entropies (entropy is bits per sample)
        double total_samples = static_cast<double>(rows) * cols * 3.0; // samples = pixels*channels
        double est_bytes_transformed = (transformedEntropy * total_samples) / 8.0;
        double est_bytes_quantized = (quantizedEntropy * total_samples) / 8.0;

        double bpp_raw = metrics::BitsPerPixelFromBytes(raw_bytes, rows, cols);
        double bpp_trans = metrics::BitsPerPixelFromBytes(static_cast<std::size_t>(est_bytes_transformed + 0.5), rows, cols);
        double bpp_quant = metrics::BitsPerPixelFromBytes(static_cast<std::size_t>(est_bytes_quantized + 0.5), rows, cols);

        std::cout << "Estimated bpp (raw/transformed/quantized): "
                  << bpp_raw << " / " << bpp_trans << " / " << bpp_quant << " bits/pixel" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Failed to compute bits-per-pixel: " << e.what() << std::endl;
    }
    
    delete transform;
    
    return 0;
}
