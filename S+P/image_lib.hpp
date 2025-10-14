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

class TransformedChunk {
private:
    int chunkSize; // Size of the square chunk
    TransformSpace transformSpace; // Current transform space
    
    // Array of channels - can access with channel[idx]
    std::vector<std::vector<std::vector<uint8_t>>> channels; // channels[channel_idx][row][col]

public:
    // Constructors
    TransformedChunk(int size = 8, TransformSpace space = TransformSpace::rawRGB) : chunkSize(size), transformSpace(space) {
        initializeChannels();
    }
    
    
    // Constructor with direct channel data
    TransformedChunk(const std::vector<std::vector<std::vector<uint8_t>>>& channelData, TransformSpace space) 
        : transformSpace(space) {
        if (channelData.size() != 3) {
            throw std::runtime_error("TransformedChunk requires exactly 3 channels");
        }
        
        chunkSize = channelData[0].size();
        if (channelData[0].empty() || channelData[0][0].size() != chunkSize) {
            throw std::runtime_error("Invalid channel data dimensions");
        }
        
        // Verify all channels have the same dimensions
        for (int ch = 0; ch < 3; ch++) {
            if (channelData[ch].size() != chunkSize) {
                throw std::runtime_error("All channels must have the same dimensions");
            }
            for (int i = 0; i < chunkSize; i++) {
                if (channelData[ch][i].size() != chunkSize) {
                    throw std::runtime_error("All channels must be square");
                }
            }
        }
        
        channels = channelData;
    }

    // Copy constructor
    TransformedChunk(const TransformedChunk& other) 
        : chunkSize(other.chunkSize), transformSpace(other.transformSpace) {
        channels = other.channels;
    }

    // Copy assignment operator
    TransformedChunk& operator=(const TransformedChunk& other) {
        if (this != &other) {
            if (chunkSize != other.chunkSize) {
                throw std::runtime_error("Cannot assign TransformedChunk with different chunk size");
            }
            transformSpace = other.transformSpace;
            channels = other.channels;
        }
        return *this;
    }

    // Accessors
    int getChunkSize() { return chunkSize; }
    TransformSpace getTransformSpace() { return transformSpace; }
    
    // Const versions of accessors
    int getChunkSize() const { return chunkSize; }
    TransformSpace getTransformSpace() const { return transformSpace; }

    // Channel accessors
    std::vector<std::vector<uint8_t>>& getChannel(int idx) { 
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return channels[idx]; 
    }
    
    const std::vector<std::vector<uint8_t>>& getChannel(int idx) const { 
        if (idx < 0 || idx >= 3) {
            throw std::runtime_error("Invalid channel index. Must be 0, 1, or 2");
        }
        return channels[idx]; 
    }

    // Set transform space (doesn't convert data, just changes interpretation)
    void setTransformSpace(TransformSpace space) {
        transformSpace = space;
    }

    // Clear all channels (set to zero)
    void clear() {
        for (int ch = 0; ch < 3; ch++) {
            for (int i = 0; i < chunkSize; i++) {
                for (int j = 0; j < chunkSize; j++) {
                    channels[ch][i][j] = 0;
                }
            }
        }
    }

    // Fill all channels with a specific value
    void fill(uint8_t value) {
        for (int ch = 0; ch < 3; ch++) {
            for (int i = 0; i < chunkSize; i++) {
                for (int j = 0; j < chunkSize; j++) {
                    channels[ch][i][j] = value;
                }
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
        channels.resize(3); // 3 channels
        for (int ch = 0; ch < 3; ch++) {
            channels[ch].resize(chunkSize, std::vector<uint8_t>(chunkSize, 0));
        }
    }
};

class TransformedChunkedImage {
private:
    int originalRows, originalColumns;
    int chunkRows, chunkColumns;
    int chunkSize;
    std::vector<std::vector<TransformedChunk>> chunks;
    TransformSpace transformSpace;

public:
    // Constructor from Image object (creates chunks and converts to TransformedChunk)
    TransformedChunkedImage(const Image& image, int size = 8, TransformSpace space = TransformSpace::rawRGB) 
        : chunkSize(size), transformSpace(space) {
        
        originalRows = image.getRows();
        originalColumns = image.getColumns();
        
        // Calculate number of chunks needed
        chunkRows = (originalRows + chunkSize - 1) / chunkSize;
        chunkColumns = (originalColumns + chunkSize - 1) / chunkSize;
        
        // Initialize chunk grid
        chunks.resize(chunkRows, std::vector<TransformedChunk>(chunkColumns));
        
        // Create chunks directly from image pixels
        for (int chunkR = 0; chunkR < chunkRows; chunkR++) {
            for (int chunkC = 0; chunkC < chunkColumns; chunkC++) {
                // Create TransformedChunk directly
                TransformedChunk transformedChunk(chunkSize, space);
                
                // Copy pixels from image to channels
                for (int i = 0; i < chunkSize; i++) {
                    for (int j = 0; j < chunkSize; j++) {
                        int imgRow = chunkR * chunkSize + i;
                        int imgCol = chunkC * chunkSize + j;
                        
                        if (imgRow < originalRows && imgCol < originalColumns) {
                            const Pixel& pixel = image.getPixel(imgRow, imgCol);
                            if (space == TransformSpace::rawRGB) {
                                // Ensure pixel is in RGB color space
                                Pixel rgbPixel = pixel;
                                if (pixel.getColorSpace() == ColorSpace::YCbCr) {
                                    rgbPixel.convertToRGB();
                                }
                                transformedChunk.getChannel(0)[i][j] = rgbPixel.getR();
                                transformedChunk.getChannel(1)[i][j] = rgbPixel.getG();
                                transformedChunk.getChannel(2)[i][j] = rgbPixel.getB();
                            } else if (space == TransformSpace::rawYCbCr) {
                                // Convert pixel to YCbCr if needed
                                Pixel ycbcrPixel = pixel;
                                if (pixel.getColorSpace() == ColorSpace::RGB) {
                                    ycbcrPixel.convertToYCbCr();
                                }
                                transformedChunk.getChannel(0)[i][j] = ycbcrPixel.getY();
                                transformedChunk.getChannel(1)[i][j] = ycbcrPixel.getCb();
                                transformedChunk.getChannel(2)[i][j] = ycbcrPixel.getCr();
                            } else {
                                // For other transform spaces, start with RGB data
                                Pixel rgbPixel = pixel;
                                if (pixel.getColorSpace() == ColorSpace::YCbCr) {
                                    rgbPixel.convertToRGB();
                                }
                                transformedChunk.getChannel(0)[i][j] = rgbPixel.getR();
                                transformedChunk.getChannel(1)[i][j] = rgbPixel.getG();
                                transformedChunk.getChannel(2)[i][j] = rgbPixel.getB();
                            }
                        } else {
                            // Pad with black pixels if image doesn't fill the chunk
                            transformedChunk.getChannel(0)[i][j] = 0;
                            transformedChunk.getChannel(1)[i][j] = 0;
                            transformedChunk.getChannel(2)[i][j] = 0;
                        }
                    }
                }
                
                chunks[chunkR][chunkC] = transformedChunk;
            }
        }
    }

    // Default constructor
    TransformedChunkedImage() : originalRows(0), originalColumns(0), chunkRows(0), chunkColumns(0), chunkSize(8), transformSpace(TransformSpace::rawRGB) {}

    // Accessors
    int getOriginalRows() { return originalRows; }
    int getOriginalColumns() { return originalColumns; }
    int getChunkRows() { return chunkRows; }
    int getChunkColumns() { return chunkColumns; }
    int getChunkSize() { return chunkSize; }
    TransformSpace getTransformSpace() { return transformSpace; }
    
    TransformedChunk& getChunk(int chunkRow, int chunkCol) { return chunks[chunkRow][chunkCol]; }

    // Set transform space for all chunks (doesn't convert data, just changes interpretation)
    void setTransformSpace(TransformSpace space) {
        transformSpace = space;
        for (int i = 0; i < chunkRows; i++) {
            for (int j = 0; j < chunkColumns; j++) {
                chunks[i][j].setTransformSpace(space);
            }
        }
    }

    // Get total number of chunks
    int getTotalChunks() {
        return chunkRows * chunkColumns;
    }

    // Get chunk at linear index (for iteration)
    TransformedChunk& getChunkAt(int index) {
        int chunkR = index / chunkColumns;
        int chunkC = index % chunkColumns;
        return chunks[chunkR][chunkC];
    }

    // Get string representation of transform space
    std::string getTransformSpaceString() {
        switch (transformSpace) {
            case TransformSpace::rawRGB: return "rawRGB";
            case TransformSpace::rawYCbCr: return "rawYCbCr";
            case TransformSpace::DCT: return "DCT";
            case TransformSpace::DWT: return "DWT";
            case TransformSpace::SP: return "SP";
            default: return "Unknown";
        }
    }
};

// Abstract base class for image transforms
class Transform {
protected:
    TransformedChunkedImage transformedChunkedImage;
    TransformSpace transformSpace;

public:
    // Constructor that takes an Image and initializes it into a TransformedChunkedImage
    Transform(const Image& image, int chunkSize = 8, TransformSpace space = TransformSpace::rawRGB) 
        : transformedChunkedImage(image, chunkSize, space), transformSpace(space) {}
    
    // Virtual destructor for proper inheritance
    virtual ~Transform() = default;
    
    // Pure virtual methods to be implemented by derived classes
    virtual TransformedChunk encodeChunk(const TransformedChunk& inputChunk) = 0;
    virtual TransformedChunk decodeChunk(const TransformedChunk& encodedChunk) = 0;
    
    // Already implemented method that applies encoding to each chunk
    TransformedChunkedImage applyTransform() {
        // Create a copy of the original chunked image
        TransformedChunkedImage result = transformedChunkedImage;
        
        // Apply encoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            TransformedChunk& inputChunk = transformedChunkedImage.getChunkAt(i);
            TransformedChunk encodedChunk = encodeChunk(inputChunk);
            
            // Copy encoded chunk to result
            TransformedChunk& resultChunk = result.getChunkAt(i);
            resultChunk = encodedChunk;
        }
        
        return result;
    }
    
    // Method to apply both encoding and decoding (for testing)
    TransformedChunkedImage applyFullTransform() {
        TransformedChunkedImage encoded = applyTransform();
        TransformedChunkedImage result = encoded;
        
        // Apply decoding to each encoded chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            TransformedChunk& encodedChunk = encoded.getChunkAt(i);
            TransformedChunk decodedChunk = decodeChunk(encodedChunk);
            
            // Copy decoded chunk to result
            TransformedChunk& resultChunk = result.getChunkAt(i);
            resultChunk = decodedChunk;
        }
        
        return result;
    }
    
    // Accessor methods
    TransformedChunkedImage& getTransformedChunkedImage() { return transformedChunkedImage; }
    TransformSpace getTransformSpace() { return transformSpace; }
    int getChunkSize() { return transformedChunkedImage.getChunkSize(); }
    int getTotalChunks() { return transformedChunkedImage.getTotalChunks(); }
    
    // Get string representation of transform space
    std::string getTransformSpaceString() {
        switch (transformSpace) {
            case TransformSpace::rawRGB: return "rawRGB";
            case TransformSpace::rawYCbCr: return "rawYCbCr";
            case TransformSpace::DCT: return "DCT";
            case TransformSpace::DWT: return "DWT";
            case TransformSpace::SP: return "SP";
            default: return "Unknown";
        }
    }
};


