#ifndef IMAGE_LIB_HPP
#define IMAGE_LIB_HPP

#include <cstdint>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// STB Image Write implementation
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

enum class TransformSpace {
    Raw,        // Raw image data
    DCT,        // Discrete Cosine Transform
    DWT,        // Discrete Wavelet Transform
    Haar,       // Haar Wavelet Transform
    SP          // S+P Transform
};

enum class ColorSpace {
    RGB,
    YCbCr
};

// Helper functions to convert enums to strings
std::string colorSpaceToString(ColorSpace colorSpace) {
    switch (colorSpace) {
        case ColorSpace::RGB: return "RGB";
        case ColorSpace::YCbCr: return "YCbCr";
        default: return "Unknown";
    }
}

std::string transformSpaceToString(TransformSpace transformSpace) {
    switch (transformSpace) {
        case TransformSpace::Raw: return "Raw";
        case TransformSpace::DCT: return "DCT";
        case TransformSpace::DWT: return "DWT";
        case TransformSpace::SP: return "SP";
        default: return "Unknown";
    }
}

class Pixel {
private:
    int val[3];

public:
    // Constructors
    Pixel() {
        val[0] = val[1] = val[2] = 0;
    }
    
    Pixel(int c1, int c2, int c3) {
        val[0] = c1;
        val[1] = c2;
        val[2] = c3;
    }

    // Accessor with bounds checking
    int& operator[](int idx) {
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return val[idx];
    }
    
    const int& operator[](int idx) const {
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return val[idx];
    }

    // Pixel difference function with scaling
    Pixel getScaledPixelDifference(const Pixel& other, int scale = 100) const {
        return Pixel(scale * abs(val[0] - other.val[0]), scale * abs(val[1] - other.val[1]), scale * abs(val[2] - other.val[2]));
    }

    // Conversion methods
    // Should only be called if the data space is RGB - managed by the Image class that contains the pixels
    void convertToYCbCr() {
        int r = val[0], g = val[1], b = val[2];
        
        // Standard RGB to YCbCr conversion as defined in the paper JPEG FIF
        int y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
        int cb = (int)(-0.1687 * r - 0.3313 * g + 0.5 * b + 128);
        int cr = (int)(0.5 * r - 0.4187 * g - 0.0813 * b + 128);
        
        val[0] = y;
        val[1] = cb;
        val[2] = cr;
    }
    // Should only be called if the data space is YCbCr - managed by the Image class that contains the pixels
    void convertToRGB() {
        int y = val[0], cb = val[1] - 128, cr = val[2] - 128;
        
        // Standard YCbCr to RGB conversion as defined in the paper JPEG FIF
        int r = (int)(y + 1.402 * cr);
        int g = (int)(y - 0.34414 * cb - 0.71414 * cr);
        int b = (int)(y + 1.772 * cb);
        
        val[0] = r;
        val[1] = g;
        val[2] = b;
    }
};

// Forward declaration
class ChunkedImage;
class Chunk;

// NOTE FOR DOCUMENTATION:
// To access the data in the pixels use pixels[row][col][channel],
// opposed to chunkedImage, where the indexing is channels[channel][row][col]
class Image {
private:
    int rows, columns;
    std::vector<std::vector<Pixel>> pixels;
    TransformSpace transformSpace;
    ColorSpace colorSpace;

public:
    // always reads RGB in from a file
    Image(const std::string& filename) : transformSpace(TransformSpace::Raw), colorSpace(ColorSpace::RGB) {
        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
        
        if (!data) {
            throw std::runtime_error("Failed to load image: " + filename);
        }
        
        // Set dimensions and resize the image data
        rows = height;
        columns = width;
        pixels.resize(rows, std::vector<Pixel>(columns));
        
        // Convert stb_image data to our Pixel format
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                pixels[y][x][0] = data[idx];
                pixels[y][x][1] = data[idx+1];
                pixels[y][x][2] = data[idx+2];
            }
        }
        
        stbi_image_free(data);
    }

    // Constructor that reconstructs Image from ChunkedImage
    Image(const ChunkedImage& chunkedImage);

    // TODO: Add a default constructor to be used by the entropy coder reader to create an image

    // Accessors
    int getRows() const { return rows; }
    int getColumns() const { return columns; }
    const Pixel& getPixel(int row, int col) const { return pixels[row][col]; }
    Pixel& getPixel(int row, int col) { return pixels[row][col]; }
    TransformSpace getTransformSpace() const { return transformSpace; }
    ColorSpace getColorSpace() const { return colorSpace; }

    // Color space conversion methods
    void convertToRGB() {
        assert(transformSpace == TransformSpace::Raw && "convertToRGB() can only be called when transformSpace is Raw");
        assert(colorSpace == ColorSpace::YCbCr && "convertToRGB() can only be called when colorSpace is YCbCr");
        
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                pixels[row][col].convertToRGB();
            }
        }
        transformSpace = TransformSpace::Raw;
        colorSpace = ColorSpace::RGB;
    }
    
    void convertToYCbCr() {
        assert(transformSpace == TransformSpace::Raw && "convertToYCbCr() can only be called when transformSpace is Raw");
        assert(colorSpace == ColorSpace::RGB && "convertToYCbCr() can only be called when colorSpace is RGB");
        
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                pixels[row][col].convertToYCbCr();
            }
        }
        transformSpace = TransformSpace::Raw;
        colorSpace = ColorSpace::YCbCr;
    }
    
    // Print method using helper functions
    void printInfo() const {
        std::cout << "Image Info:" << std::endl;
        std::cout << "  Dimensions: " << rows << "x" << columns << std::endl;
        std::cout << "  Transform Space: " << transformSpaceToString(transformSpace) << std::endl;
        std::cout << "  Color Space: " << colorSpaceToString(colorSpace) << std::endl;
    }
    
    // Calculate entropy of the image
    double getEntropy() const {
        // Count frequency of each pixel value using map (supports any range)
        std::map<int, int> frequency;
        int totalValues = rows * columns * 3; // Total number of values across all channels
        
        // Count frequencies across all channels
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                for (int channel = 0; channel < 3; channel++) {
                    int value = pixels[row][col][channel];
                    frequency[value]++;
                }
            }
        }
        
        // Calculate entropy
        double entropy = 0.0;
        for (const auto& [value, count] : frequency) {
            double probability = static_cast<double>(count) / totalValues;
            entropy -= probability * std::log2(probability);
        }
        
        return entropy;
    }
    
    // Save image as PNG file (only works when image is in RGB format)
    bool saveAsPNG(const std::string& filename) const {
        // Check if the image is in RGB format
        if (colorSpace != ColorSpace::RGB) {
            std::cerr << "Error: Image must be in RGB format to save as PNG. Current color space: " 
                      << colorSpaceToString(colorSpace) << std::endl;
            return false;
        }
        
        // Check if the image is in Raw transform space
        // COMMENTED OUT FOR NOW - allow saving any transform space
        /*
        if (transformSpace != TransformSpace::Raw) {
            std::cerr << "Error: Image must be in Raw transform space to save as PNG. Current transform space: " 
                      << transformSpaceToString(transformSpace) << std::endl;
            return false;
        }
        */
        
        // Convert pixel data to flat RGB array for STB with clamping
        std::vector<unsigned char> rgbData(rows * columns * 3);
        int idx = 0;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                // Clamp values to [0, 255] range
                rgbData[idx++] = static_cast<unsigned char>(std::min(255, std::max(0, pixels[row][col][0]))); // R
                rgbData[idx++] = static_cast<unsigned char>(std::min(255, std::max(0, pixels[row][col][1]))); // G
                rgbData[idx++] = static_cast<unsigned char>(std::min(255, std::max(0, pixels[row][col][2]))); // B
            }
        }
        
        // Write PNG file using STB
        int result = stbi_write_png(filename.c_str(), columns, rows, 3, rgbData.data(), columns * 3);
        
        if (result == 0) {
            std::cerr << "Error: Failed to write PNG file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Successfully saved image as PNG: " << filename << std::endl;
        return true;
    }
    
    // Save individual channel as black and white image
    bool saveChannelAsBW(const std::string& filename, int channel, int scale = 1) const {
        // Check if the image is in RGB format
        if (colorSpace != ColorSpace::RGB) {
            std::cerr << "Error: Image must be in RGB format to save channel as BW. Current color space: " 
                      << colorSpaceToString(colorSpace) << std::endl;
            return false;
        }
        
        // Check if the image is in Raw transform space
        // COMMENTED OUT FOR NOW - allow saving any transform space
        /*
        if (transformSpace != TransformSpace::Raw) {
            std::cerr << "Error: Image must be in Raw transform space to save channel as BW. Current transform space: " 
                      << transformSpaceToString(transformSpace) << std::endl;
            return false;
        }
        */
        
        // Validate channel index
        if (channel < 0 || channel >= 3) {
            std::cerr << "Error: Channel index must be 0 (R), 1 (G), or 2 (B)" << std::endl;
            return false;
        }
        
        // Convert pixel data to flat grayscale array for STB with clamping
        std::vector<unsigned char> grayData(rows * columns);
        int idx = 0;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                int value = pixels[row][col][channel] * scale;
                // Clamp value to valid range [0, 255]
                value = std::min(255, std::max(0, value));
                grayData[idx++] = static_cast<unsigned char>(value);
            }
        }
        
        // Write PNG file using STB (grayscale, 1 channel)
        int result = stbi_write_png(filename.c_str(), columns, rows, 1, grayData.data(), columns);
        
        if (result == 0) {
            std::cerr << "Error: Failed to write PNG file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Successfully saved channel " << channel << " as BW PNG: " << filename << std::endl;
        return true;
    }
    
    // Save all channels as separate black and white images
    bool saveAllChannelsAsBW(const std::string& baseFilename, int scale = 1) const {
        std::string channelNames[] = {"R", "G", "B"};
        bool allSuccess = true;
        
        for (int ch = 0; ch < 3; ch++) {
            std::string filename = baseFilename + "_" + channelNames[ch] + ".png";
            if (!saveChannelAsBW(filename, ch, scale)) {
                allSuccess = false;
            }
        }
        
        return allSuccess;
    }
};

// Function to compute pixel-wise difference between two images
Image imageDiff(const Image& img1, const Image& img2, int scale = 100) {
    // Check if images have the same dimensions
    if (img1.getRows() != img2.getRows() || img1.getColumns() != img2.getColumns()) {
        throw std::runtime_error("Images must have the same dimensions for difference computation");
    }
    
    // Check if both images are in the same color space
    if (img1.getColorSpace() != img2.getColorSpace()) {
        throw std::runtime_error("Images must be in the same color space for difference computation");
    }
    
    // Check if both images are in Raw transform space
    // COMMENTED OUT FOR NOW - allow difference computation for any transform space
    /*
    if (img1.getTransformSpace() != TransformSpace::Raw || img2.getTransformSpace() != TransformSpace::Raw) {
        throw std::runtime_error("Both images must be in Raw transform space for difference computation");
    }
    */
    
    // Create result image with same dimensions and properties as input images
    Image result = img1; // Copy constructor to get same dimensions and properties
    
    // Compute pixel-wise difference
    for (int row = 0; row < img1.getRows(); row++) {
        for (int col = 0; col < img1.getColumns(); col++) {
            const Pixel& pixel1 = img1.getPixel(row, col);
            const Pixel& pixel2 = img2.getPixel(row, col);
            
            // Use the scaled pixel difference function to compute difference
            Pixel diffPixel = pixel1.getScaledPixelDifference(pixel2, scale);
            
            // Set the difference pixel in the result image
            result.getPixel(row, col) = diffPixel;
        }
    }
    
    return result;
}

class Chunk {
private:
    int chunkSize; // Size of the square chunk
    
    // Array of channels - can access with channel[idx], always exactly 3
    std::vector<std::vector<std::vector<int>>> channels; // channels[channel_idx][row][col]

public:
    // Constructor, only initializes but values must be set by fetching each channel by reference
    Chunk(int chunkSize) : chunkSize(chunkSize) {
        channels.resize(3); // 3 channels
        for (int ch = 0; ch < 3; ch++)    // Get chunk at linear index (for iteration)
            channels[ch].resize(chunkSize, std::vector<int>(chunkSize, 0));
    }
    
    int getChunkSize() const { return chunkSize; }

    // Channel accessors with bounds checking
    std::vector<std::vector<int>>& operator[](int idx) { 
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return channels[idx]; 
    }
    
    const std::vector<std::vector<int>>& operator[](int idx) const { 
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return channels[idx]; 
    }
};

class ChunkedImage {
private:
    int originalRows, originalColumns; // how many rows and columns of PIXELS there are
    int chunkRows, chunkColumns; // how many rows and columns of CHUNKS there are
    int chunkSize;
    std::vector<std::vector<Chunk>> chunks;
    TransformSpace transformSpace; // which transform space we are using, RGB, YCbCr, DCT, DWT, SP
    ColorSpace colorSpace; // which color space we are using, RGB or YCbCr

public:
    // Constructor from Image object
    ChunkedImage(const Image& image, int chunkSize) 
        : chunkSize(chunkSize), transformSpace(image.getTransformSpace()), colorSpace(image.getColorSpace()) {
        
        originalRows = image.getRows();
        originalColumns = image.getColumns();
        
        // Calculate number of chunks needed
        chunkRows = (originalRows + chunkSize - 1) / chunkSize;
        chunkColumns = (originalColumns + chunkSize - 1) / chunkSize;
        
        // Initialize chunk grid
        chunks.resize(chunkRows, std::vector<Chunk>(chunkColumns, Chunk(chunkSize)));
        
        // Create chunks directly from image pixels
        for (int chunkR = 0; chunkR < chunkRows; chunkR++) {
            for (int chunkC = 0; chunkC < chunkColumns; chunkC++) {
                // Create Chunk directly
                Chunk imageChunk(chunkSize);

                // Copy pixels from image to channels
                for (int i = 0; i < chunkSize; i++) {
                    for (int j = 0; j < chunkSize; j++) {
                        int imgRow = chunkR * chunkSize + i;
                        int imgCol = chunkC * chunkSize + j;
                        if (imgRow < originalRows && imgCol < originalColumns) {
                            const Pixel& pixel = image.getPixel(imgRow, imgCol);
                            for(int ch = 0; ch < 3; ch++)
                                imageChunk[ch][i][j] = pixel[ch]; // access the channels with operator[]
                        } else {
                            // Pad with black pixels if image doesn't fill the chunk
                            for(int ch = 0; ch < 3; ch++)
                                imageChunk[ch][i][j] = 0;
                        }
                    }
                }
            
                chunks[chunkR][chunkC] = imageChunk;
            }
        }
    }
    
    // Constructor with explicit parameters
    ChunkedImage(int originalRows, int originalColumns, TransformSpace transformSpace, ColorSpace colorSpace, int chunkSize = 8)
        : originalRows(originalRows), originalColumns(originalColumns), 
          chunkSize(chunkSize), transformSpace(transformSpace), colorSpace(colorSpace) {
        
        // Calculate number of chunks needed
        chunkRows = (originalRows + chunkSize - 1) / chunkSize;
        chunkColumns = (originalColumns + chunkSize - 1) / chunkSize;
        
        // Initialize chunk grid with empty chunks
        chunks.resize(chunkRows, std::vector<Chunk>(chunkColumns, Chunk(chunkSize)));
    }

    // Accessors
    int getOriginalRows() const { return originalRows; }
    int getOriginalColumns() const { return originalColumns; }
    int getChunkRows() const { return chunkRows; }
    int getChunkColumns() const { return chunkColumns; }
    int getChunkSize() const { return chunkSize; }
    TransformSpace getTransformSpace() const { return transformSpace; }
    ColorSpace getColorSpace() const { return colorSpace; }
    
    Chunk& getChunk(int chunkRow, int chunkCol) { return chunks[chunkRow][chunkCol]; }
    const Chunk& getChunk(int chunkRow, int chunkCol) const { return chunks[chunkRow][chunkCol]; }

    // Get total number of chunks
    int getTotalChunks() const {
        return chunkRows * chunkColumns;
    }

    // Get chunk at linear index (for iteration)
    const Chunk& getChunkAt(int index) const {
        int chunkR = index / chunkColumns;
        int chunkC = index % chunkColumns;
        return chunks[chunkR][chunkC];
    }

    // Get chunk at linear index (for iteration)
    Chunk& getChunkAt(int index) {
        int chunkR = index / chunkColumns;
        int chunkC = index % chunkColumns;
        return chunks[chunkR][chunkC];
    }
    
    // Auxiliary function to create a fresh ChunkedImage with same parameters
    // this is used to create a fresh ChunkedImage with same parameters to store the result of the transform
    ChunkedImage createFreshCopyForTransformResult(TransformSpace resultTransformSpace) const {
        return ChunkedImage(originalRows, originalColumns, resultTransformSpace, colorSpace, chunkSize);
    }
    
    // Print method using helper functions
    void printInfo() const {
        std::cout << "ChunkedImage Info:" << std::endl;
        std::cout << "  Original Dimensions: " << originalRows << "x" << originalColumns << std::endl;
        std::cout << "  Chunk Dimensions: " << chunkRows << "x" << chunkColumns << std::endl;
        std::cout << "  Chunk Size: " << chunkSize << "x" << chunkSize << std::endl;
        std::cout << "  Total Chunks: " << getTotalChunks() << std::endl;
        std::cout << "  Transform Space: " << transformSpaceToString(transformSpace) << std::endl;
        std::cout << "  Color Space: " << colorSpaceToString(colorSpace) << std::endl;
    }
};

// Implementation of Image constructor that takes ChunkedImage
inline Image::Image(const ChunkedImage& chunkedImage) : 
    rows(chunkedImage.getOriginalRows()), 
    columns(chunkedImage.getOriginalColumns()),
    transformSpace(chunkedImage.getTransformSpace()),
    colorSpace(chunkedImage.getColorSpace()) {
    
    pixels.resize(rows, std::vector<Pixel>(columns));
    
    int chunkSize = chunkedImage.getChunkSize();
    int chunkRows = chunkedImage.getChunkRows();
    int chunkColumns = chunkedImage.getChunkColumns();
    
    // Copy data from chunks back to pixel array
    for (int chunkRow = 0; chunkRow < chunkRows; chunkRow++) {
        for (int chunkCol = 0; chunkCol < chunkColumns; chunkCol++) {
            const Chunk& chunk = chunkedImage.getChunk(chunkRow, chunkCol);
            
            // Calculate the starting position in the original image
            int startRow = chunkRow * chunkSize;
            int startCol = chunkCol * chunkSize;
            
            // Copy pixels from this chunk
            for (int localRow = 0; localRow < chunkSize; localRow++) {
                for (int localCol = 0; localCol < chunkSize; localCol++) {
                    int globalRow = startRow + localRow;
                    int globalCol = startCol + localCol;
                    
                    // Only copy if within original image bounds
                    if (globalRow < rows && globalCol < columns) {
                        // Copy all three channels
                        for (int channel = 0; channel < 3; channel++) {
                            pixels[globalRow][globalCol][channel] = chunk[channel][localRow][localCol];
                        }
                    }
                }
            }
        }
    }
}

// Abstract base class for image transforms
class Transform {
protected:
    TransformSpace transformSpace;

    // Protected constructor - derived classes must call this and set transformSpace
    Transform(TransformSpace transformSpace) : transformSpace(transformSpace) {}

    // Pure virtual methods to be implemented by derived classes
    // These are protected so only derived classes can call them
    virtual void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) = 0;
    virtual void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) = 0;
	
	virtual std::vector<std::vector<int>> getQuantizationMatrix() {
		
		std::vector<std::vector<int>> quantizationMatrix = { {16, 11, 10, 16, 24, 40, 51, 61},
																 {12, 12, 14, 19, 26, 58, 60, 55},
																 {14, 13, 16, 24, 40, 57, 69, 56},
																 {14, 17, 22, 29, 51, 87, 80, 62},
																 {18, 22, 37, 56, 68, 109, 103, 77},
																 {24, 35, 55, 64, 81, 104, 113, 92},
																 {49, 64, 78, 87, 103, 121, 120, 101},
																 {72, 92, 95, 98, 112, 100, 103, 99} };

		return quantizationMatrix;
		
	}
	
	virtual void quantizeChunk(const Chunk& inputChunk, Chunk& outputChunk) {
		
		int size = inputChunk.getChunkSize();
		int result;
		int inputValue;
		int matrixValue;
		
		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
					inputValue = inputChunk[ch][u][v];
					matrixValue = getQuantizationMatrix()[u][v];
					result = std::round(inputValue / matrixValue);
					outputChunk[ch][u][v] = result;
				}
			}
		}
	}
	virtual void dequantizeChunk(const Chunk& encodedChunk, Chunk& outputChunk) {
		
		int size = encodedChunk.getChunkSize();
		int result;
		int encodedValue;
		int matrixValue;
		
		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
					encodedValue = encodedChunk[ch][u][v];
					matrixValue = getQuantizationMatrix()[u][v];
					result = encodedValue * matrixValue;
					outputChunk[ch][u][v] = result;
				}
			}
		}
	}
	


public:
    // Virtual destructor for proper inheritance
    virtual ~Transform() = default;

    // Apply transform to a chunked image
    ChunkedImage applyTransform(const ChunkedImage& chunkedImage) {
        // Check that the chunkedImage is in Raw transform space
        if (chunkedImage.getTransformSpace() != TransformSpace::Raw) {
            throw std::runtime_error(
                "ChunkedImage transform space (" + transformSpaceToString(chunkedImage.getTransformSpace()) + 
                ") is not Raw. Transform can only be applied to Raw data."
            );
        }
        
        // Create a fresh ChunkedImage with same parameters but in the transform's final transform space
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(transformSpace);
        
        // Apply encoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            encodeChunk(inputChunk, resultChunk);
        }
        
        return result;
    }
    
    // Apply inverse transform to a chunked image (decoding)
    ChunkedImage applyInverseTransform(const ChunkedImage& chunkedImage) {
        // Check that the chunkedImage is in the correct transform space
        if (chunkedImage.getTransformSpace() != transformSpace) {
            throw std::runtime_error(
                "ChunkedImage transform space (" + transformSpaceToString(chunkedImage.getTransformSpace()) + 
                ") does not match transform final transform space (" + transformSpaceToString(transformSpace) + "). Necessary for inverse transform."
            );
        }
        
        // Create a fresh ChunkedImage with same parameters, but now untransformed
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(TransformSpace::Raw);
        
        // Apply decoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            decodeChunk(inputChunk, resultChunk);
        }
        
        return result;
    }
	
	ChunkedImage applyQuantization(const ChunkedImage& chunkedImage) {
        // Allow quantization for any chunk size; concrete Transform implementations
        // are responsible for supporting the provided chunk dimensions.
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(transformSpace);

        // Apply quantization to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            quantizeChunk(inputChunk, resultChunk);
        }

        return result;

	}
	
	ChunkedImage applyInverseQuantization(const ChunkedImage& chunkedImage) {
        // Allow inverse-quantization for any chunk size; concrete Transform implementations
        // are responsible for supporting the provided chunk dimensions.
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(transformSpace);

        // Apply inverse quantization to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            dequantizeChunk(inputChunk, resultChunk);
        }

        return result;

	}
    
    // Accessor methods
    TransformSpace getTransformSpace() const { return transformSpace; }
};

#endif // IMAGE_LIB_HPP


