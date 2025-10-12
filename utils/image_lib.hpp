#include <cstdint>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum class ColorSpace {
    RGB,
    YCbCr
};

enum class TransformSpace {
    rawRGB,     // Raw RGB values
    rawYCbCr,   // Raw YCbCr values
    DCT,        // Discrete Cosine Transform coefficients
    DWT,        // Discrete Wavelet Transform coefficients
    SP          // Subband Processing coefficients
};

class Pixel {
private:
    uint8_t val[3];
    ColorSpace colorSpace;

public:
    // Constructors
    Pixel() : colorSpace(ColorSpace::RGB) {
        val[0] = val[1] = val[2] = 0;
    }
    
    Pixel(uint8_t r, uint8_t g, uint8_t b) : colorSpace(ColorSpace::RGB) {
        val[0] = r;
        val[1] = g;
        val[2] = b;
    }
    
    Pixel(uint8_t y, uint8_t cb, uint8_t cr, ColorSpace cs) : colorSpace(cs) {
        val[0] = y;
        val[1] = cb;
        val[2] = cr;
    }

    // Accessors
    uint8_t getR() const { 
        if (colorSpace != ColorSpace::RGB)
            throw std::runtime_error("Cannot get R value: pixel is not in RGB color space");
        return val[0]; 
    }
    uint8_t getG() const { 
        if (colorSpace != ColorSpace::RGB)
            throw std::runtime_error("Cannot get G value: pixel is not in RGB color space");
        return val[1]; 
    }
    uint8_t getB() const { 
        if (colorSpace != ColorSpace::RGB)
            throw std::runtime_error("Cannot get B value: pixel is not in RGB color space");
        return val[2]; 
    }
    
    uint8_t getY() const { 
        if (colorSpace != ColorSpace::YCbCr)
            throw std::runtime_error("Cannot get Y value: pixel is not in YCbCr color space");
        return val[0]; 
    }
    uint8_t getCb() const { 
        if (colorSpace != ColorSpace::YCbCr)
            throw std::runtime_error("Cannot get Cb value: pixel is not in YCbCr color space");
        return val[1]; 
    }
    uint8_t getCr() const { 
        if (colorSpace != ColorSpace::YCbCr)
            throw std::runtime_error("Cannot get Cr value: pixel is not in YCbCr color space");
        return val[2]; 
    }
    
    ColorSpace getColorSpace() const { return colorSpace; }

    // Setters
    void setRGB(uint8_t r, uint8_t g, uint8_t b) {
        val[0] = r;
        val[1] = g;
        val[2] = b;
        colorSpace = ColorSpace::RGB;
    }
    
    void setYCbCr(uint8_t y, uint8_t cb, uint8_t cr) {
        val[0] = y;
        val[1] = cb;
        val[2] = cr;
        colorSpace = ColorSpace::YCbCr;
    }

    // Conversion methods
    void convertToYCbCr() {
        if (colorSpace == ColorSpace::YCbCr) return;
        
        uint8_t r = val[0], g = val[1], b = val[2];
        
        // Standard RGB to YCbCr conversion as defined in the paper JPEG FIF
        int y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
        int cb = (int)(-0.1687 * r - 0.3313 * g + 0.5 * b + 128);
        int cr = (int)(0.5 * r - 0.4187 * g - 0.0813 * b + 128);
        
        // Clamp values to valid range
        val[0] = (uint8_t)std::max(0, std::min(255, y));
        val[1] = (uint8_t)std::max(0, std::min(255, cb));
        val[2] = (uint8_t)std::max(0, std::min(255, cr));
        
        colorSpace = ColorSpace::YCbCr;
    }
    
    void convertToRGB() {
        if (colorSpace == ColorSpace::RGB) return;
        
        int y = val[0], cb = val[1] - 128, cr = val[2] - 128;
        
        // Standard YCbCr to RGB conversion
        int r = (int)(y + 1.402 * cr);
        int g = (int)(y - 0.34414 * cb - 0.71414 * cr);
        int b = (int)(y + 1.772 * cb);
        
        // Clamp values to valid range
        val[0] = (uint8_t)std::max(0, std::min(255, r));
        val[1] = (uint8_t)std::max(0, std::min(255, g));
        val[2] = (uint8_t)std::max(0, std::min(255, b));
        
        colorSpace = ColorSpace::RGB;
    }
};

class Image {
private:
    int rows, columns;
    std::vector<std::vector<Pixel>> pixels;

public:
    // Constructors
    Image() : rows(0), columns(0) {}
    
    Image(int _rows, int _columns) : rows(_rows), columns(_columns) {
        pixels.resize(rows, std::vector<Pixel>(columns));
    }

    // Accessors
    int getRows() const { return rows; }
    int getColumns() const { return columns; }
    Pixel& getPixel(int row, int col) { return pixels[row][col]; }
    const Pixel& getPixel(int row, int col) const { return pixels[row][col]; }

    // File I/O
    bool read_from_file(const std::string& filename) {
        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
        
        if (!data) {
            std::cerr << "Failed to load image: " << filename << std::endl;
            return false;
        }
        
        // Resize the image data
        rows = height;
        columns = width;
        pixels.resize(rows, std::vector<Pixel>(columns));
        
        // Convert stb_image data to our Pixel format
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                pixels[y][x].setRGB(data[idx], data[idx+1], data[idx+2]);
            }
        }
        
        stbi_image_free(data);
        return true;
    }
};

class ImageChunk {
private:
    int chunkSize; // Variable chunk size, set once during construction
    std::vector<std::vector<Pixel>> pixels;
    int chunkRow, chunkCol; // Position of this chunk in the original image

public:
    // Constructors
    ImageChunk(int size = 8) : chunkSize(size), chunkRow(0), chunkCol(0) {
        pixels.resize(chunkSize, std::vector<Pixel>(chunkSize));
    }
    
    ImageChunk(int size, int row, int col) : chunkSize(size), chunkRow(row), chunkCol(col) {
        pixels.resize(chunkSize, std::vector<Pixel>(chunkSize));
    }

    // Accessors
    int getChunkSize() const { return chunkSize; }
    int getChunkRow() const { return chunkRow; }
    int getChunkCol() const { return chunkCol; }
    Pixel& getPixel(int row, int col) { return pixels[row][col]; }
    const Pixel& getPixel(int row, int col) const { return pixels[row][col]; }

    // Set pixel at local coordinates within the chunk
    void setPixel(int row, int col, const Pixel& pixel) {
        if (row >= 0 && row < chunkSize && col >= 0 && col < chunkSize) {
            pixels[row][col] = pixel;
        }
    }

    // Convert all pixels in chunk to specified color space
    void convertToColorSpace(ColorSpace targetSpace) {
        for (int i = 0; i < chunkSize; i++) {
            for (int j = 0; j < chunkSize; j++) {
                if (targetSpace == ColorSpace::YCbCr) {
                    pixels[i][j].convertToYCbCr();
                } else {
                    pixels[i][j].convertToRGB();
                }
            }
        }
    }
};

class ChunkedImage {
private:
    int originalRows, originalColumns;
    int chunkRows, chunkColumns;
    int chunkSize;
    std::vector<std::vector<ImageChunk>> chunks;
    ColorSpace colorSpace;

public:
    // Constructor from Image object
    ChunkedImage(const Image& image, int size = 8) : chunkSize(size) {
        originalRows = image.getRows();
        originalColumns = image.getColumns();
        colorSpace = ColorSpace::RGB; // Default, will be updated if needed
        
        // Calculate number of chunks needed
        chunkRows = (originalRows + chunkSize - 1) / chunkSize;
        chunkColumns = (originalColumns + chunkSize - 1) / chunkSize;
        
        // Initialize chunk grid
        chunks.resize(chunkRows, std::vector<ImageChunk>(chunkColumns));
        
        // Populate chunks from image
        for (int chunkR = 0; chunkR < chunkRows; chunkR++) {
            for (int chunkC = 0; chunkC < chunkColumns; chunkC++) {
                chunks[chunkR][chunkC] = ImageChunk(chunkSize, chunkR, chunkC);
                
                // Copy pixels from image to chunk
                for (int i = 0; i < chunkSize; i++) {
                    for (int j = 0; j < chunkSize; j++) {
                        int imgRow = chunkR * chunkSize + i;
                        int imgCol = chunkC * chunkSize + j;
                        
                        if (imgRow < originalRows && imgCol < originalColumns) {
                            chunks[chunkR][chunkC].setPixel(i, j, image.getPixel(imgRow, imgCol));
                        } else {
                            // Pad with black pixels if image doesn't fill the chunk
                            chunks[chunkR][chunkC].setPixel(i, j, Pixel(0, 0, 0));
                        }
                    }
                }
            }
        }
    }

    // Default constructor
    ChunkedImage() : originalRows(0), originalColumns(0), chunkRows(0), chunkColumns(0), chunkSize(8), colorSpace(ColorSpace::RGB) {}

    // Accessors
    int getOriginalRows() const { return originalRows; }
    int getOriginalColumns() const { return originalColumns; }
    int getChunkRows() const { return chunkRows; }
    int getChunkColumns() const { return chunkColumns; }
    int getChunkSize() const { return chunkSize; }
    ColorSpace getColorSpace() const { return colorSpace; }
    ImageChunk& getChunk(int chunkRow, int chunkCol) { return chunks[chunkRow][chunkCol]; }
    const ImageChunk& getChunk(int chunkRow, int chunkCol) const { return chunks[chunkRow][chunkCol]; }

    // Convert all chunks to specified color space
    void convertToColorSpace(ColorSpace targetSpace) {
        colorSpace = targetSpace;
        for (int i = 0; i < chunkRows; i++) {
            for (int j = 0; j < chunkColumns; j++) {
                chunks[i][j].convertToColorSpace(targetSpace);
            }
        }
    }

    // Convert back to Image object
    Image toImage() const {
        Image result(originalRows, originalColumns);
        
        for (int chunkR = 0; chunkR < chunkRows; chunkR++) {
            for (int chunkC = 0; chunkC < chunkColumns; chunkC++) {
                const ImageChunk& chunk = chunks[chunkR][chunkC];
                
                for (int i = 0; i < chunkSize; i++) {
                    for (int j = 0; j < chunkSize; j++) {
                        int imgRow = chunkR * chunkSize + i;
                        int imgCol = chunkC * chunkSize + j;
                        
                        if (imgRow < originalRows && imgCol < originalColumns) {
                            result.getPixel(imgRow, imgCol) = chunk.getPixel(i, j);
                        }
                    }
                }
            }
        }
        
        return result;
    }

    // Get total number of chunks
    int getTotalChunks() const {
        return chunkRows * chunkColumns;
    }

    // Get chunk at linear index (for iteration)
    ImageChunk& getChunkAt(int index) {
        int chunkR = index / chunkColumns;
        int chunkC = index % chunkColumns;
        return chunks[chunkR][chunkC];
    }

    const ImageChunk& getChunkAt(int index) const {
        int chunkR = index / chunkColumns;
        int chunkC = index % chunkColumns;
        return chunks[chunkR][chunkC];
    }
};

class TransformedChunk {
private:
    int chunkSize; // Size of the square chunk
    int chunkRow, chunkCol; // Position of this chunk in the original image
    TransformSpace transformSpace; // Current transform space
    
    // Separate grids for each color channel
    std::vector<std::vector<uint8_t>> channel1; // R or Y channel
    std::vector<std::vector<uint8_t>> channel2; // G or Cb channel
    std::vector<std::vector<uint8_t>> channel3; // B or Cr channel

public:
    // Constructors
    TransformedChunk(int size = 8) : chunkSize(size), chunkRow(0), chunkCol(0), transformSpace(TransformSpace::rawRGB) {
        initializeChannels();
    }
    
    TransformedChunk(int size, int row, int col, TransformSpace space = TransformSpace::rawRGB) 
        : chunkSize(size), chunkRow(row), chunkCol(col), transformSpace(space) {
        initializeChannels();
    }

    // Copy constructor
    TransformedChunk(const TransformedChunk& other) 
        : chunkSize(other.chunkSize), chunkRow(other.chunkRow), chunkCol(other.chunkCol), 
          transformSpace(other.transformSpace) {
        channel1 = other.channel1;
        channel2 = other.channel2;
        channel3 = other.channel3;
    }

    // Copy assignment operator
    TransformedChunk& operator=(const TransformedChunk& other) {
        if (this != &other) {
            if (chunkSize != other.chunkSize) {
                throw std::runtime_error("Cannot assign TransformedChunk with different chunk size");
            }
            chunkRow = other.chunkRow;
            chunkCol = other.chunkCol;
            transformSpace = other.transformSpace;
            channel1 = other.channel1;
            channel2 = other.channel2;
            channel3 = other.channel3;
        }
        return *this;
    }

    // Accessors
    int getChunkSize() const { return chunkSize; }
    int getChunkRow() const { return chunkRow; }
    int getChunkCol() const { return chunkCol; }
    TransformSpace getTransformSpace() const { return transformSpace; }

    // Channel accessors
    std::vector<std::vector<uint8_t>>& getChannel1() { return channel1; }
    const std::vector<std::vector<uint8_t>>& getChannel1() const { return channel1; }
    
    std::vector<std::vector<uint8_t>>& getChannel2() { return channel2; }
    const std::vector<std::vector<uint8_t>>& getChannel2() const { return channel2; }
    
    std::vector<std::vector<uint8_t>>& getChannel3() { return channel3; }
    const std::vector<std::vector<uint8_t>>& getChannel3() const { return channel3; }

    // RGB channel accessors (for rawRGB transform space)
    std::vector<std::vector<uint8_t>>& getRChannel() { 
        if (transformSpace != TransformSpace::rawRGB) {
            throw std::runtime_error("Cannot get R channel: not in rawRGB transform space");
        }
        return channel1; 
    }
    
    std::vector<std::vector<uint8_t>>& getGChannel() { 
        if (transformSpace != TransformSpace::rawRGB) {
            throw std::runtime_error("Cannot get G channel: not in rawRGB transform space");
        }
        return channel2; 
    }
    
    std::vector<std::vector<uint8_t>>& getBChannel() { 
        if (transformSpace != TransformSpace::rawRGB) {
            throw std::runtime_error("Cannot get B channel: not in rawRGB transform space");
        }
        return channel3; 
    }

    // YCbCr channel accessors (for rawYCbCr transform space)
    std::vector<std::vector<uint8_t>>& getYChannel() { 
        if (transformSpace != TransformSpace::rawYCbCr) {
            throw std::runtime_error("Cannot get Y channel: not in rawYCbCr transform space");
        }
        return channel1; 
    }
    
    std::vector<std::vector<uint8_t>>& getCbChannel() { 
        if (transformSpace != TransformSpace::rawYCbCr) {
            throw std::runtime_error("Cannot get Cb channel: not in rawYCbCr transform space");
        }
        return channel2; 
    }
    
    std::vector<std::vector<uint8_t>>& getCrChannel() { 
        if (transformSpace != TransformSpace::rawYCbCr) {
            throw std::runtime_error("Cannot get Cr channel: not in rawYCbCr transform space");
        }
        return channel3; 
    }

    // DCT coefficient accessors (for DCT transform space)
    std::vector<std::vector<uint8_t>>& getDCTChannel1() { 
        if (transformSpace != TransformSpace::DCT) {
            throw std::runtime_error("Cannot get DCT channel 1: not in DCT transform space");
        }
        return channel1; 
    }
    
    std::vector<std::vector<uint8_t>>& getDCTChannel2() { 
        if (transformSpace != TransformSpace::DCT) {
            throw std::runtime_error("Cannot get DCT channel 2: not in DCT transform space");
        }
        return channel2; 
    }
    
    std::vector<std::vector<uint8_t>>& getDCTChannel3() { 
        if (transformSpace != TransformSpace::DCT) {
            throw std::runtime_error("Cannot get DCT channel 3: not in DCT transform space");
        }
        return channel3; 
    }

    // Set individual coefficient value
    void setCoefficient(int channel, int row, int col, uint8_t value) {
        if (channel < 1 || channel > 3) {
            throw std::runtime_error("Invalid channel number. Must be 1, 2, or 3");
        }
        if (row < 0 || row >= chunkSize || col < 0 || col >= chunkSize) {
            throw std::runtime_error("Invalid coefficient position");
        }
        
        switch (channel) {
            case 1: channel1[row][col] = value; break;
            case 2: channel2[row][col] = value; break;
            case 3: channel3[row][col] = value; break;
        }
    }

    // Get individual coefficient value
    uint8_t getCoefficient(int channel, int row, int col) const {
        if (channel < 1 || channel > 3) {
            throw std::runtime_error("Invalid channel number. Must be 1, 2, or 3");
        }
        if (row < 0 || row >= chunkSize || col < 0 || col >= chunkSize) {
            throw std::runtime_error("Invalid coefficient position");
        }
        
        switch (channel) {
            case 1: return channel1[row][col];
            case 2: return channel2[row][col];
            case 3: return channel3[row][col];
            default: return 0;
        }
    }

    // Set transform space (doesn't convert data, just changes interpretation)
    void setTransformSpace(TransformSpace space) {
        transformSpace = space;
    }

    // Clear all channels (set to zero)
    void clear() {
        for (int i = 0; i < chunkSize; i++) {
            for (int j = 0; j < chunkSize; j++) {
                channel1[i][j] = 0;
                channel2[i][j] = 0;
                channel3[i][j] = 0;
            }
        }
    }

    // Fill all channels with a specific value
    void fill(uint8_t value) {
        for (int i = 0; i < chunkSize; i++) {
            for (int j = 0; j < chunkSize; j++) {
                channel1[i][j] = value;
                channel2[i][j] = value;
                channel3[i][j] = value;
            }
        }
    }

    // Get string representation of transform space
    std::string getTransformSpaceString() const {
        switch (transformSpace) {
            case TransformSpace::rawRGB: return "rawRGB";
            case TransformSpace::rawYCbCr: return "rawYCbCr";
            case TransformSpace::DCT: return "DCT";
            case TransformSpace::DWT: return "DWT";
            case TransformSpace::SP: return "SP";
            default: return "Unknown";
        }
    }

private:
    // Initialize all channels with zeros
    void initializeChannels() {
        channel1.resize(chunkSize, std::vector<uint8_t>(chunkSize, 0));
        channel2.resize(chunkSize, std::vector<uint8_t>(chunkSize, 0));
        channel3.resize(chunkSize, std::vector<uint8_t>(chunkSize, 0));
    }
};
