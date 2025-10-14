#include <cstdint>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum class DataSpace {
    RGB,        // Raw RGB
    YCbCr,      // Raw YCbCr
    DCT,        // Discrete Cosine Transform
    DWT,        // Discrete Wavelet Transform
    SP          // S+P Transform
};

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
    DataSpace dataSpace;

public:
    // always reads RGB in from a file
    Image(const std::string& filename) : dataSpace(DataSpace::RGB) {
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

    // Accessors
    int getRows() const { return rows; }
    int getColumns() const { return columns; }
    const Pixel& getPixel(int row, int col) const { return pixels[row][col]; }
    DataSpace getDataSpace() const { return dataSpace; }

    // Color space conversion methods
    void convertToRGB() {
        assert(dataSpace == DataSpace::YCbCr && "convertToRGB() can only be called when dataSpace is YCbCr");
        
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                pixels[row][col].convertToRGB();
            }
        }
        dataSpace = DataSpace::RGB;
    }
    
    void convertToYCbCr() {
        assert(dataSpace == DataSpace::RGB && "convertToYCbCr() can only be called when dataSpace is RGB");
        
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                pixels[row][col].convertToYCbCr();
            }
        }
        dataSpace = DataSpace::YCbCr;
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
    DataSpace dataSpace; // which data space we are using, RGB, YCbCr, DCT, DWT, SP

public:
    // Constructor from Image object
    ChunkedImage(const Image& image, int chunkSize = 8) 
        : chunkSize(chunkSize), dataSpace(image.getDataSpace()) {
        
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
    ChunkedImage(int originalRows, int originalColumns, DataSpace dataSpace, int chunkSize = 8)
        : originalRows(originalRows), originalColumns(originalColumns), 
          chunkSize(chunkSize), dataSpace(dataSpace) {
        
        // Calculate number of chunks needed
        chunkRows = (originalRows + chunkSize - 1) / chunkSize;
        chunkColumns = (originalColumns + chunkSize - 1) / chunkSize;
        
        // Initialize chunk grid with empty chunks
        chunks.resize(chunkRows, std::vector<Chunk>(chunkColumns, Chunk(chunkSize)));
    }

    // Accessors
    int getOriginalRows() { return originalRows; }
    int getOriginalColumns() { return originalColumns; }
    int getChunkRows() { return chunkRows; }
    int getChunkColumns() { return chunkColumns; }
    int getChunkSize() { return chunkSize; }
    DataSpace getDataSpace() const { return dataSpace; }
    
    Chunk& getChunk(int chunkRow, int chunkCol) { return chunks[chunkRow][chunkCol]; }

    // Get total number of chunks
    int getTotalChunks() {
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

    // Get string representation of data space
    std::string getDataSpaceString() {
        switch (dataSpace) {
            case DataSpace::RGB: return "RGB";
            case DataSpace::YCbCr: return "YCbCr";
            case DataSpace::DCT: return "DCT";
            case DataSpace::DWT: return "DWT";
            case DataSpace::SP: return "SP";
            default: return "Unknown";
        }
    }
};

// Abstract base class for image transforms
class Transform {
protected:
    DataSpace finalDataSpace;

    // Protected constructor - derived classes must call this and set finalDataSpace
    Transform(DataSpace dataSpace) : finalDataSpace(dataSpace) {}

    // Pure virtual methods to be implemented by derived classes
    // These are protected so only derived classes can call them
    virtual Chunk encodeChunk(const Chunk& inputChunk) = 0;
    virtual Chunk decodeChunk(const Chunk& encodedChunk) = 0;

public:
    // Virtual destructor for proper inheritance
    virtual ~Transform() = default;

    // Apply transform to a chunked image
    ChunkedImage applyTransform(const ChunkedImage& chunkedImage) {
        // Create a copy of the original chunked image
        ChunkedImage result = chunkedImage;
        
        // Apply encoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk encodedChunk = encodeChunk(inputChunk);
            
            // Copy encoded chunk to result
            Chunk& resultChunk = result.getChunkAt(i);
            resultChunk = encodedChunk;
        }
        
        return result;
    }
    
    // Method to apply both encoding and decoding (for testing)
    ChunkedImage applyFullTransform(const ChunkedImage& chunkedImage) {
        ChunkedImage encoded = applyTransform(chunkedImage);
        ChunkedImage result = encoded;
        
        // Apply decoding to each encoded chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& encodedChunk = encoded.getChunkAt(i);
            Chunk decodedChunk = decodeChunk(encodedChunk);
            
            // Copy decoded chunk to result
            Chunk& resultChunk = result.getChunkAt(i);
            resultChunk = decodedChunk;
        }
        
        return result;
    }
    
    // Accessor methods
    DataSpace getDataSpace() { return finalDataSpace; }
    
    // Get string representation of data space
    std::string getDataSpaceString() {
        switch (finalDataSpace) {
            case DataSpace::RGB: return "RGB";
            case DataSpace::YCbCr: return "YCbCr";
            case DataSpace::DCT: return "DCT";
            case DataSpace::DWT: return "DWT";
            case DataSpace::SP: return "SP";
            default: return "Unknown";
        }
    }
};


