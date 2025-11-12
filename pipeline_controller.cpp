#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sys/stat.h>
#include "utils/image_lib.hpp"
#include "transforms/dct_transform.hpp"
#include "transforms/dft_transform.hpp"
#include "transforms/sp_transform.hpp"
#include "transforms/haar_transform.hpp"  // Uncomment when Haar transform is fixed

// Forward declarations
struct ProgramArgs {
    int chunkSize;
    std::string imagePath;
    std::string transformType;
};

// Function declarations
ProgramArgs parseCommandLineArgs(int argc, char* argv[]);
void printUsage(const char* programName);
double loadAndDisplayImageInfo(const std::string& imagePath, int chunkSize);
ChunkedImage createChunkedImage(const Image& img, int chunkSize);
Transform* createTransform(const std::string& transformType);
ChunkedImage applyTransform(Transform* transform, const ChunkedImage& chunkedImg, const std::string& transformType);
void analyzeEncodedImage(const ChunkedImage& encodedResult, const Image& encodedImg, int chunkSize);
void createEncodedVisualization(const Image& encodedImg, const std::string& saveDir);
ChunkedImage applyInverseTransform(Transform* transform, const ChunkedImage& encodedResult, const std::string& transformType);
void saveDecodedImage(const Image& decodedImg, const std::string& saveDir);
void computeAndAnalyzeDifference(const Image& originalImg, const Image& decodedImg, const std::string& saveDir, int chunkSize);
void printEntropySummary(double originalEntropy, double encodedEntropy, double quantizedEntropy, double decodedEntropy, const std::string& transformType);

int main(int argc, char* argv[]) {
    // Parse command line arguments
    ProgramArgs args = parseCommandLineArgs(argc, argv);
    if (args.chunkSize == -1) {
        return 1; // Error already printed
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
    
    std::cout << "Starting " << args.transformType << " Transform test with chunk size: " << args.chunkSize << std::endl;
    
    // Load image and display basic info
    Image img(args.imagePath);
    double originalEntropy = loadAndDisplayImageInfo(args.imagePath, args.chunkSize);
    
    // Create ChunkedImage
    ChunkedImage chunkedImg = createChunkedImage(img, args.chunkSize);
    
    // Create and apply transform
    Transform* transform = createTransform(args.transformType);
    ChunkedImage encodedResult = applyTransform(transform, chunkedImg, args.transformType);
    
    // Calculate entropy of encoded image (before quantization)
    Image encodedImg(encodedResult);
    double encodedEntropy = encodedImg.getEntropy();
    std::cout << "Encoded image entropy (before quantization): " << encodedEntropy << " bits per pixel" << std::endl;
    
    // Apply quantization with scale 1
    std::cout << "\nApplying quantization with scale 1..." << std::endl;
    ChunkedImage quantizedResult = transform->applyQuantization(encodedResult, 1.0);
    std::cout << "Quantization applied successfully!" << std::endl;
    
    // Calculate entropy of quantized image
    Image quantizedImg(quantizedResult);
    double quantizedEntropy = quantizedImg.getEntropy();
    std::cout << "Quantized image entropy: " << quantizedEntropy << " bits per pixel" << std::endl;
    
    // Analyze encoded image (using quantized result)
    analyzeEncodedImage(quantizedResult, quantizedImg, args.chunkSize);
    
    // Create visualization (using quantized result)
    createEncodedVisualization(quantizedImg, saveDir);
    
    // Apply inverse quantization with scale 1
    std::cout << "\nApplying inverse quantization with scale 1..." << std::endl;
    ChunkedImage dequantizedResult = transform->applyInverseQuantization(quantizedResult, 1.0);
    std::cout << "Inverse quantization applied successfully!" << std::endl;
    
    // Apply inverse transform
    ChunkedImage decodedResult = applyInverseTransform(transform, dequantizedResult, args.transformType);
    
    // Calculate entropy of decoded image
    Image decodedImg(decodedResult);
    double decodedEntropy = decodedImg.getEntropy();
    std::cout << "Decoded image entropy: " << decodedEntropy << " bits per pixel" << std::endl;
    
    // Save decoded image
    saveDecodedImage(decodedImg, saveDir);
    
    // Compute and analyze difference
    computeAndAnalyzeDifference(img, decodedImg, saveDir, args.chunkSize);
    
    // Print entropy summary
    printEntropySummary(originalEntropy, encodedEntropy, quantizedEntropy, decodedEntropy, args.transformType);
    
    std::cout << "\n" << args.transformType << " Transform test completed successfully!" << std::endl;
    
    // Clean up transform object
    delete transform;
    
    return 0;
}

// Function implementations

ProgramArgs parseCommandLineArgs(int argc, char* argv[]) {
    ProgramArgs args;
    args.chunkSize = -1;
    args.imagePath = "";
    args.transformType = "DCT"; // Default
    
    if (argc < 3 || argc > 7) {
        printUsage(argv[0]);
        return args; // Return with error state
    }
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--chunksize" && i + 1 < argc) {
            try {
                args.chunkSize = std::stoi(argv[i + 1]);
                if (args.chunkSize <= 0) {
                    throw std::invalid_argument("Chunk size must be positive");
                }
                i++; // Skip the next argument since we consumed it
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid chunk size. " << e.what() << std::endl;
                args.chunkSize = -1; // Set error state
                return args;
            }
        } else if (arg == "--path" && i + 1 < argc) {
            args.imagePath = argv[i + 1];
            i++; // Skip the next argument since we consumed it
        } else if (arg == "--transform" && i + 1 < argc) {
            std::string transformArg = argv[i + 1];
            // Convert to uppercase for case-insensitive comparison
            std::transform(transformArg.begin(), transformArg.end(), transformArg.begin(), ::toupper);
            if (transformArg == "DCT" || transformArg == "SP" || transformArg == "HAAR" || transformArg == "DFT") {
                args.transformType = transformArg;
                i++; // Skip the next argument since we consumed it
            } else {
                std::cerr << "Error: Invalid transform type '" << argv[i + 1] << "'. Must be DCT, DFT, SP, or HAAR." << std::endl;
                args.chunkSize = -1; // Set error state
                return args;
            }
        } else if (arg == "--chunksize") {
            std::cerr << "Error: --chunksize requires a value" << std::endl;
            args.chunkSize = -1; // Set error state
            return args;
        } else if (arg == "--path") {
            std::cerr << "Error: --path requires a value" << std::endl;
            args.chunkSize = -1; // Set error state
            return args;
        } else if (arg == "--transform") {
            std::cerr << "Error: --transform requires a value (DCT, DFT, SP, or HAAR)" << std::endl;
            args.chunkSize = -1; // Set error state
            return args;
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            args.chunkSize = -1; // Set error state
            return args;
        }
    }
    
    // Validate required arguments
    if (args.chunkSize == -1) {
        std::cerr << "Error: --chunksize is required" << std::endl;
        return args;
    }
    
    if (args.imagePath.empty()) {
        std::cerr << "Error: --path is required" << std::endl;
        args.chunkSize = -1; // Set error state
        return args;
    }
    
    // Check if image file exists
    if (!std::filesystem::exists(args.imagePath)) {
        std::cerr << "Error: Image file '" << args.imagePath << "' does not exist" << std::endl;
        args.chunkSize = -1; // Set error state
        return args;
    }
    
    return args;
}

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " --chunksize <size> --path <image_path> [--transform DCT|DFT|SP|HAAR]" << std::endl;
    std::cerr << "Example: " << programName << " --chunksize 8 --path Datasets/KodakImages/1.png" << std::endl;
    std::cerr << "Example: " << programName << " --chunksize 8 --path Datasets/KodakImages/1.png --transform SP" << std::endl;
    std::cerr << "Example: " << programName << " --chunksize 8 --path Datasets/KodakImages/1.png --transform HAAR" << std::endl;
    std::cerr << "Required arguments:" << std::endl;
    std::cerr << "  --chunksize <size>     : Size of chunks (must be positive)" << std::endl;
    std::cerr << "  --path <image_path>    : Path to input image" << std::endl;
    std::cerr << "Optional arguments:" << std::endl;
    std::cerr << "  --transform <type>     : Transform type (DCT, DFT, SP, HAAR). Default: DCT" << std::endl;
}

double loadAndDisplayImageInfo(const std::string& imagePath, int chunkSize) {
    std::cout << "Loading image from: " << imagePath << std::endl;
    Image img(imagePath);
    std::cout << "Image loaded successfully!" << std::endl;
    img.printInfo();
    
    // Print some original pixel values (first chunk block)
    if (chunkSize <= 16) {
        std::cout << "\nOriginal image values (first " << chunkSize << "x" << chunkSize << " block, R channel):" << std::endl;
        for (int i = 0; i < chunkSize && i < img.getRows(); i++) {
            for (int j = 0; j < chunkSize && j < img.getColumns(); j++) {
                std::cout << img.getPixel(i, j)[0] << "\t";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "\nOriginal image values (first 8x8 block, R channel) - chunk too large to display fully:" << std::endl;
        for (int i = 0; i < 8 && i < img.getRows(); i++) {
            for (int j = 0; j < 8 && j < img.getColumns(); j++) {
                std::cout << img.getPixel(i, j)[0] << "\t";
            }
            std::cout << std::endl;
        }
    }
    
    // Calculate entropy of original image
    std::cout << "\nCalculating entropy of original image..." << std::endl;
    double originalEntropy = img.getEntropy();
    std::cout << "Original image entropy: " << originalEntropy << " bits per pixel" << std::endl;
    
    return originalEntropy;
}

ChunkedImage createChunkedImage(const Image& img, int chunkSize) {
    std::cout << "\nCreating ChunkedImage..." << std::endl;
    ChunkedImage chunkedImg(img, chunkSize);
    std::cout << "ChunkedImage created successfully!" << std::endl;
    chunkedImg.printInfo();
    return chunkedImg;
}

Transform* createTransform(const std::string& transformType) {
    std::cout << "\nCreating " << transformType << " Transform..." << std::endl;
    Transform* transform = nullptr;
    
    if (transformType == "DCT") {
        transform = new DCTTransform();
    } else if (transformType == "DFT") {
        transform = new DFTTransform();
    } else if (transformType == "SP") {
        transform = new SPTransform();
    } else if (transformType == "HAAR") {
        transform = new HaarTransform();
    } else {
        std::cerr << "Error: Unknown transform type: " << transformType << std::endl;
        return nullptr;
    }
    
    std::cout << transformType << " Transform created successfully!" << std::endl;
    std::cout << "Transform space: " << transformSpaceToString(transform->getTransformSpace()) << std::endl;
    
    return transform;
}

ChunkedImage applyTransform(Transform* transform, const ChunkedImage& chunkedImg, const std::string& transformType) {
    std::cout << "\nApplying " << transformType << " transform (encoding)..." << std::endl;
    ChunkedImage encodedResult = transform->applyTransform(chunkedImg);
    std::cout << transformType << " transform applied successfully!" << std::endl;
    std::cout << "Encoded result info:" << std::endl;
    encodedResult.printInfo();
    return encodedResult;
}

void analyzeEncodedImage(const ChunkedImage& encodedResult, const Image& encodedImg, int chunkSize) {
    std::cout << "\n=== ENCODED IMAGE ANALYTICS ===" << std::endl;
    
    // Collect all pixel values for each channel
    std::vector<std::vector<int>> channelValues(3);
    for (int ch = 0; ch < 3; ch++) {
        channelValues[ch].reserve(encodedImg.getRows() * encodedImg.getColumns());
    }
    
    for (int row = 0; row < encodedImg.getRows(); row++) {
        for (int col = 0; col < encodedImg.getColumns(); col++) {
            const Pixel& pixel = encodedImg.getPixel(row, col);
            for (int ch = 0; ch < 3; ch++) {
                channelValues[ch].push_back(pixel[ch]);
            }
        }
    }
    
    // Calculate statistics for each channel
    const char* channelNames[] = {"Red", "Green", "Blue"};
    for (int ch = 0; ch < 3; ch++) {
        std::vector<int>& values = channelValues[ch];
        std::sort(values.begin(), values.end());
        
        int n = values.size();
        std::cout << "\n" << channelNames[ch] << " Channel Statistics:" << std::endl;
        std::cout << "  Total pixels: " << n << std::endl;
        std::cout << "  Minimum: " << values[0] << std::endl;
        std::cout << "  Maximum: " << values[n-1] << std::endl;
        std::cout << "  Range: " << (values[n-1] - values[0]) << std::endl;
        
        // Percentiles
        std::cout << "  Percentiles:" << std::endl;
        std::cout << "    Top 1%:  " << values[static_cast<int>(n * 0.99)] << std::endl;
        std::cout << "    Top 5%:  " << values[static_cast<int>(n * 0.95)] << std::endl;
        std::cout << "    Top 10%: " << values[static_cast<int>(n * 0.90)] << std::endl;
        std::cout << "    Top 25%: " << values[static_cast<int>(n * 0.75)] << std::endl;
        std::cout << "    Median:   " << values[static_cast<int>(n * 0.50)] << std::endl;
        std::cout << "    Bottom 25%: " << values[static_cast<int>(n * 0.25)] << std::endl;
        std::cout << "    Bottom 10%: " << values[static_cast<int>(n * 0.10)] << std::endl;
        std::cout << "    Bottom 5%:  " << values[static_cast<int>(n * 0.05)] << std::endl;
        std::cout << "    Bottom 1%:  " << values[static_cast<int>(n * 0.01)] << std::endl;
        
        // Calculate mean and standard deviation
        double sum = 0.0;
        for (int val : values) {
            sum += val;
        }
        double mean = sum / n;
        
        double variance = 0.0;
        for (int val : values) {
            variance += (val - mean) * (val - mean);
        }
        double stdDev = std::sqrt(variance / n);
        
        std::cout << "  Mean: " << mean << std::endl;
        std::cout << "  Std Dev: " << stdDev << std::endl;
        
        // Count values near zero (important for compression)
        int nearZeroCount = 0;
        int zeroCount = 0;
        for (int val : values) {
            if (val == 0) zeroCount++;
            if (std::abs(val) <= 1) nearZeroCount++;
        }
        
        std::cout << "  Zero values: " << zeroCount << " (" << (100.0 * zeroCount / n) << "%)" << std::endl;
        std::cout << "  Near zero (±1): " << nearZeroCount << " (" << (100.0 * nearZeroCount / n) << "%)" << std::endl;
        
        // Energy concentration analysis
        long long totalEnergy = 0;
        long long top10Energy = 0;
        int top10Count = static_cast<int>(n * 0.10);
        
        for (int i = 0; i < n; i++) {
            long long energy = static_cast<long long>(values[i]) * values[i];
            totalEnergy += energy;
            if (i >= n - top10Count) {
                top10Energy += energy;
            }
        }
        
        double energyConcentration = (totalEnergy > 0) ? (100.0 * top10Energy / totalEnergy) : 0.0;
        std::cout << "  Energy in top 10%: " << energyConcentration << "%" << std::endl;
    }
    
    // Cross-channel analysis
    std::cout << "\nCross-Channel Analysis:" << std::endl;
    
    // Calculate correlation between channels
    double correlations[3][3];
    for (int ch1 = 0; ch1 < 3; ch1++) {
        for (int ch2 = 0; ch2 < 3; ch2++) {
            if (ch1 == ch2) {
                correlations[ch1][ch2] = 1.0;
            } else {
                // Calculate Pearson correlation
                double sum1 = 0, sum2 = 0, sum1sq = 0, sum2sq = 0, sum12 = 0;
                int n = channelValues[ch1].size();
                
                for (int i = 0; i < n; i++) {
                    double val1 = channelValues[ch1][i];
                    double val2 = channelValues[ch2][i];
                    sum1 += val1;
                    sum2 += val2;
                    sum1sq += val1 * val1;
                    sum2sq += val2 * val2;
                    sum12 += val1 * val2;
                }
                
                double numerator = n * sum12 - sum1 * sum2;
                double denominator = std::sqrt((n * sum1sq - sum1 * sum1) * (n * sum2sq - sum2 * sum2));
                correlations[ch1][ch2] = (denominator != 0) ? (numerator / denominator) : 0.0;
            }
        }
    }
    
    std::cout << "  Channel Correlations:" << std::endl;
    std::cout << "    R-G: " << correlations[0][1] << std::endl;
    std::cout << "    R-B: " << correlations[0][2] << std::endl;
    std::cout << "    G-B: " << correlations[1][2] << std::endl;
    
    // Transform-specific analysis
    std::cout << "\nTransform-Specific Analysis:" << std::endl;
    
    // Analyze first chunk in detail
    const Chunk& firstChunk = encodedResult.getChunkAt(0);
    std::cout << "  First chunk (" << chunkSize << "x" << chunkSize << ") analysis:" << std::endl;
    
    for (int ch = 0; ch < 3; ch++) {
        std::vector<int> chunkValues;
        chunkValues.reserve(chunkSize * chunkSize);
        
        for (int i = 0; i < chunkSize; i++) {
            for (int j = 0; j < chunkSize; j++) {
                chunkValues.push_back(firstChunk[ch][i][j]);
            }
        }
        
        std::sort(chunkValues.begin(), chunkValues.end());
        int chunkN = chunkValues.size();
        
        std::cout << "    " << channelNames[ch] << " channel:" << std::endl;
        std::cout << "      Range: [" << chunkValues[0] << ", " << chunkValues[chunkN-1] << "]" << std::endl;
        std::cout << "      DC component: " << firstChunk[ch][0][0] << std::endl;
        
        // Count non-zero coefficients
        int nonZeroCount = 0;
        for (int val : chunkValues) {
            if (val != 0) nonZeroCount++;
        }
        std::cout << "      Non-zero coefficients: " << nonZeroCount << "/" << chunkN 
                  << " (" << (100.0 * nonZeroCount / chunkN) << "%)" << std::endl;
    }
    
    // Compression potential analysis
    std::cout << "\nCompression Potential Analysis:" << std::endl;
    
    // Count coefficients by magnitude ranges
    int ranges[5] = {0}; // 0, 1-10, 11-50, 51-100, >100
    const char* rangeNames[] = {"0", "1-10", "11-50", "51-100", ">100"};
    
    for (int ch = 0; ch < 3; ch++) {
        for (int val : channelValues[ch]) {
            int absVal = std::abs(val);
            if (absVal == 0) ranges[0]++;
            else if (absVal <= 10) ranges[1]++;
            else if (absVal <= 50) ranges[2]++;
            else if (absVal <= 100) ranges[3]++;
            else ranges[4]++;
        }
    }
    
    int totalCoeffs = channelValues[0].size() * 3;
    std::cout << "  Coefficient magnitude distribution:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "    " << rangeNames[i] << ": " << ranges[i] 
                  << " (" << (100.0 * ranges[i] / totalCoeffs) << "%)" << std::endl;
    }
    
    // Theoretical compression ratio estimate
    double compressionRatio = (double)totalCoeffs / (ranges[0] + ranges[1] + ranges[2]);
    std::cout << "  Estimated compression ratio (if quantizing >50): " << compressionRatio << ":1" << std::endl;
    
    // Print some encoded values (first chunk, R channel)
    if (chunkSize <= 16) {
        std::cout << "\nEncoded values (first chunk, R channel):" << std::endl;
        for (int i = 0; i < chunkSize; i++) {
            for (int j = 0; j < chunkSize; j++) {
                std::cout << firstChunk[0][i][j] << "\t";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "\nEncoded values (first 8x8 block, R channel) - chunk too large to display fully:" << std::endl;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                std::cout << firstChunk[0][i][j] << "\t";
            }
            std::cout << std::endl;
        }
    }
}

void createEncodedVisualization(const Image& encodedImg, const std::string& saveDir) {
    std::cout << "\nCreating encoded image visualization..." << std::endl;
    
    // Collect all pixel values for each channel to calculate percentiles
    std::vector<std::vector<int>> channelValues(3);
    for (int ch = 0; ch < 3; ch++) {
        channelValues[ch].reserve(encodedImg.getRows() * encodedImg.getColumns());
    }
    
    for (int row = 0; row < encodedImg.getRows(); row++) {
        for (int col = 0; col < encodedImg.getColumns(); col++) {
            const Pixel& pixel = encodedImg.getPixel(row, col);
            for (int ch = 0; ch < 3; ch++) {
                channelValues[ch].push_back(pixel[ch]);
            }
        }
    }
    
    // Calculate 10% and 90% percentiles for scaling
    int percentile10[3], percentile90[3];
    const char* channelNames[] = {"Red", "Green", "Blue"};
    for (int ch = 0; ch < 3; ch++) {
        std::sort(channelValues[ch].begin(), channelValues[ch].end());
        int n = channelValues[ch].size();
        percentile10[ch] = channelValues[ch][static_cast<int>(n * 0.10)];
        percentile90[ch] = channelValues[ch][static_cast<int>(n * 0.90)];
    }
    
    // Create visualization image by copying the encoded image
    Image encodedVisImg = encodedImg;
    
    for (int row = 0; row < encodedImg.getRows(); row++) {
        for (int col = 0; col < encodedImg.getColumns(); col++) {
            for (int ch = 0; ch < 3; ch++) {
                int val = encodedImg.getPixel(row, col)[ch];
                
                // Scale to 0-255 range using 10%-90% percentiles
                if (percentile90[ch] == percentile10[ch]) {
                    // Avoid division by zero
                    encodedVisImg.getPixel(row, col)[ch] = 128;
                } else {
                    // Clamp to percentile range first
                    val = std::max(percentile10[ch], std::min(percentile90[ch], val));
                    
                    // Scale to 0-255
                    double scaled = (double)(val - percentile10[ch]) / (percentile90[ch] - percentile10[ch]);
                    encodedVisImg.getPixel(row, col)[ch] = static_cast<int>(scaled * 255);
                }
            }
        }
    }
    
    // Save visualization image
    std::string encodedVisPath = saveDir + "/encodedVisualization.png";
    if (encodedVisImg.saveAsPNG(encodedVisPath)) {
        std::cout << "Encoded image visualization saved successfully as " << encodedVisPath << std::endl;
        std::cout << "Scaling info:" << std::endl;
        for (int ch = 0; ch < 3; ch++) {
            std::cout << "  " << channelNames[ch] << " channel: [" << percentile10[ch] << ", " << percentile90[ch] << "] -> [0, 255]" << std::endl;
        }
    } else {
        std::cout << "Failed to save encoded image visualization" << std::endl;
    }
    
    // Save individual channel visualizations
    std::cout << "Saving encoded image channel visualizations..." << std::endl;
    std::string encodedVisBasePath = saveDir + "/encodedVisualization";
    if (encodedVisImg.saveAllChannelsAsBW(encodedVisBasePath, 1)) {
        std::cout << "Encoded image channel visualizations saved successfully as BW images in " << saveDir << std::endl;
    } else {
        std::cout << "Failed to save some encoded image channel visualizations" << std::endl;
    }
}

ChunkedImage applyInverseTransform(Transform* transform, const ChunkedImage& encodedResult, const std::string& transformType) {
    std::cout << "\nApplying inverse " << transformType << " transform (decoding)..." << std::endl;
    ChunkedImage decodedResult = transform->applyInverseTransform(encodedResult);
    std::cout << "Inverse " << transformType << " transform applied successfully!" << std::endl;
    std::cout << "Decoded result info:" << std::endl;
    decodedResult.printInfo();
    return decodedResult;
}

void saveDecodedImage(const Image& decodedImg, const std::string& saveDir) {
    // Save decoded image as PNG
    std::cout << "\nSaving decoded image as PNG..." << std::endl;
    std::string decodedPath = saveDir + "/decodedImage.png";
    if (decodedImg.saveAsPNG(decodedPath)) {
        std::cout << "Decoded image saved successfully as " << decodedPath << std::endl;
    } else {
        std::cout << "Failed to save decoded image as PNG" << std::endl;
    }
    
    // Save decoded image channels as separate black and white images
    std::cout << "Saving decoded image channels as separate BW images..." << std::endl;
    std::string decodedBasePath = saveDir + "/decodedImage";
    if (decodedImg.saveAllChannelsAsBW(decodedBasePath, 1)) {
        std::cout << "Decoded image channels saved successfully as BW images in " << saveDir << std::endl;
    } else {
        std::cout << "Failed to save some decoded image channels as BW images" << std::endl;
    }
}

void computeAndAnalyzeDifference(const Image& originalImg, const Image& decodedImg, const std::string& saveDir, int chunkSize) {
    std::cout << "\nComputing difference image between original and decoded..." << std::endl;
    try {
        Image diffImg = imageDiff(originalImg, decodedImg, 5);
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
        if (chunkSize <= 16) {
            std::cout << "\nDifference image values (first " << chunkSize << "x" << chunkSize << " block, R channel):" << std::endl;
            for (int i = 0; i < chunkSize && i < diffImg.getRows(); i++) {
                for (int j = 0; j < chunkSize && j < diffImg.getColumns(); j++) {
                    std::cout << diffImg.getPixel(i, j)[0] << "\t";
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << "\nDifference image values (first 8x8 block, R channel) - chunk too large to display fully:" << std::endl;
            for (int i = 0; i < 8 && i < diffImg.getRows(); i++) {
                for (int j = 0; j < 8 && j < diffImg.getColumns(); j++) {
                    std::cout << diffImg.getPixel(i, j)[0] << "\t";
                }
                std::cout << std::endl;
            }
        }
        
        // Analyze pixel differences between original and decoded images
        std::cout << "\n=== PIXEL ANALYSIS ===" << std::endl;
        
        // Initialize max differences for each channel
        int maxDiff[3] = {0, 0, 0};
        int minPixel[3] = {255, 255, 255};
        int maxPixel[3] = {0, 0, 0};
        int outOfBoundsCount = 0;
        
        // Analyze all pixels
        for (int row = 0; row < originalImg.getRows(); row++) {
            for (int col = 0; col < originalImg.getColumns(); col++) {
                const Pixel& origPixel = originalImg.getPixel(row, col);
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
        int totalPixels = originalImg.getRows() * originalImg.getColumns();
        
        for (int row = 0; row < originalImg.getRows(); row++) {
            for (int col = 0; col < originalImg.getColumns(); col++) {
                const Pixel& origPixel = originalImg.getPixel(row, col);
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
}

void printEntropySummary(double originalEntropy, double encodedEntropy, double quantizedEntropy, double decodedEntropy, const std::string& transformType) {
    std::cout << "\n=== ENTROPY SUMMARY ===" << std::endl;
    std::cout << "Original image entropy:  " << originalEntropy << " bits per pixel" << std::endl;
    std::cout << "Encoded image entropy:   " << encodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Quantized image entropy: " << quantizedEntropy << " bits per pixel" << std::endl;
    std::cout << "Decoded image entropy:   " << decodedEntropy << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->enc):     " << (encodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (enc->quant):    " << (quantizedEntropy - encodedEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (quant->dec):    " << (decodedEntropy - quantizedEntropy) << " bits per pixel" << std::endl;
    std::cout << "Entropy change (orig->dec):     " << (decodedEntropy - originalEntropy) << " bits per pixel" << std::endl;
}