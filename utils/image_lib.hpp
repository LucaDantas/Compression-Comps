#ifndef IMAGE_LIB_HPP
#define IMAGE_LIB_HPP

#include <cstdint>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum class TransformSpace {
    Raw,        // Raw image data
    DCT,        // Discrete Cosine Transform
    DWT,        // Discrete Wavelet Transform
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

    // TODO: Add a constructor that can take in a transformed image

    // Accessors
    int getRows() const { return rows; }
    int getColumns() const { return columns; }
    const Pixel& getPixel(int row, int col) const { return pixels[row][col]; }
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
};

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
    
    // Accessor methods
    TransformSpace getTransformSpace() const { return transformSpace; }
};

#endif // IMAGE_LIB_HPP


