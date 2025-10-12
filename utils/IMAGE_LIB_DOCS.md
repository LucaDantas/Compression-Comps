# Image Library Documentation

## Overview

The Image Library provides a comprehensive set of classes for handling image data, color space conversions, and image chunking operations. This library is designed specifically for image compression applications, particularly JPEG encoding and decoding.

## Dependencies

- **stb_image.h**: For loading images from various formats (JPEG, PNG, BMP, TGA, etc.)
- **Standard C++ Libraries**: `<cstdint>`, `<algorithm>`, `<vector>`, `<string>`, `<stdexcept>`, `<iostream>`

## Core Classes

### 1. ColorSpace Enum

```cpp
enum class ColorSpace {
    RGB,
    YCbCr
};
```

**Purpose**: Defines the available color spaces for pixel data.

- **RGB**: Standard Red-Green-Blue color space
- **YCbCr**: Luminance-Chrominance color space used in JPEG compression

---

### 2. Pixel Class

The `Pixel` class represents a single pixel with support for multiple color spaces and automatic conversion.

#### Private Members
- `uint8_t val[3]`: Internal storage for pixel values
- `ColorSpace colorSpace`: Current color space of the pixel

#### Constructors

```cpp
Pixel()                                    // Default: black RGB pixel
Pixel(uint8_t r, uint8_t g, uint8_t b)    // RGB constructor
Pixel(uint8_t y, uint8_t cb, uint8_t cr, ColorSpace cs)  // YCbCr constructor
```

#### Accessor Methods

**RGB Accessors** (throw runtime_error if not in RGB mode):
```cpp
uint8_t getR() const    // Get red component
uint8_t getG() const    // Get green component  
uint8_t getB() const    // Get blue component
```

**YCbCr Accessors** (throw runtime_error if not in YCbCr mode):
```cpp
uint8_t getY() const    // Get luminance component
uint8_t getCb() const   // Get blue chrominance component
uint8_t getCr() const   // Get red chrominance component
```

**General Accessors**:
```cpp
ColorSpace getColorSpace() const  // Get current color space
```

#### Setter Methods

```cpp
void setRGB(uint8_t r, uint8_t g, uint8_t b)           // Set RGB values
void setYCbCr(uint8_t y, uint8_t cb, uint8_t cr)       // Set YCbCr values
```

#### Conversion Methods

```cpp
void convertToYCbCr()   // Convert RGB to YCbCr using ITU-R BT.601 coefficients
void convertToRGB()     // Convert YCbCr to RGB using inverse transformation
```

**Conversion Formulas**:
- **RGB to YCbCr**: 
  - Y = 0.299×R + 0.587×G + 0.114×B
  - Cb = -0.1687×R - 0.3313×G + 0.5×B + 128
  - Cr = 0.5×R - 0.4187×G - 0.0813×B + 128

- **YCbCr to RGB**:
  - R = Y + 1.402×(Cr-128)
  - G = Y - 0.34414×(Cb-128) - 0.71414×(Cr-128)
  - B = Y + 1.772×(Cb-128)

---

### 3. Image Class

The `Image` class represents a 2D image as a collection of pixels with file I/O capabilities.

#### Private Members
- `int rows, columns`: Image dimensions
- `std::vector<std::vector<Pixel>> pixels`: 2D pixel array

#### Constructors

```cpp
Image()                           // Default: empty image
Image(int _rows, int _columns)    // Create image with specified dimensions
```

#### Accessor Methods

```cpp
int getRows() const                    // Get number of rows
int getColumns() const                 // Get number of columns
Pixel& getPixel(int row, int col)      // Get pixel reference (non-const)
const Pixel& getPixel(int row, int col) const  // Get pixel reference (const)
```

#### File I/O Methods

```cpp
bool read_from_file(const std::string& filename)  // Load image from file
```

**Supported Formats**: JPEG, PNG, BMP, TGA, and other formats supported by stb_image
**Behavior**: 
- Loads image as RGB by default
- Returns `false` if loading fails
- Automatically resizes internal storage to match loaded image

---

### 4. ImageChunk Class

The `ImageChunk` class represents a square block of pixels, typically used for JPEG compression (8×8 blocks).

#### Private Members
- `int chunkSize`: Size of the square chunk (e.g., 8 for 8×8 blocks)
- `std::vector<std::vector<Pixel>> pixels`: 2D pixel array for the chunk
- `int chunkRow, chunkCol`: Position of this chunk in the original image

#### Constructors

```cpp
ImageChunk(int size = 8)                    // Default: 8×8 chunk
ImageChunk(int size, int row, int col)      // Specify size and position
```

#### Accessor Methods

```cpp
int getChunkSize() const                    // Get chunk size
int getChunkRow() const                     // Get chunk row position
int getChunkCol() const                     // Get chunk column position
Pixel& getPixel(int row, int col)           // Get pixel within chunk
const Pixel& getPixel(int row, int col) const  // Get pixel within chunk (const)
```

#### Manipulation Methods

```cpp
void setPixel(int row, int col, const Pixel& pixel)  // Set pixel within chunk
void convertToColorSpace(ColorSpace targetSpace)     // Convert all pixels in chunk
```

---

### 5. ChunkedImage Class

The `ChunkedImage` class represents an image divided into chunks, essential for JPEG compression algorithms.

#### Private Members
- `int originalRows, originalColumns`: Original image dimensions
- `int chunkRows, chunkColumns`: Number of chunks in each dimension
- `int chunkSize`: Size of each square chunk
- `std::vector<std::vector<ImageChunk>> chunks`: 2D array of chunks
- `ColorSpace colorSpace`: Current color space of the chunked image

#### Constructors

```cpp
ChunkedImage()                                    // Default: empty chunked image
ChunkedImage(const Image& image, int size = 8)    // Create from Image with specified chunk size
```

#### Accessor Methods

```cpp
int getOriginalRows() const                       // Get original image rows
int getOriginalColumns() const                    // Get original image columns
int getChunkRows() const                          // Get number of chunk rows
int getChunkColumns() const                       // Get number of chunk columns
int getChunkSize() const                          // Get chunk size
ColorSpace getColorSpace() const                  // Get current color space
ImageChunk& getChunk(int chunkRow, int chunkCol)  // Get chunk reference
const ImageChunk& getChunk(int chunkRow, int chunkCol) const  // Get chunk reference (const)
```

#### Utility Methods

```cpp
int getTotalChunks() const                        // Get total number of chunks
ImageChunk& getChunkAt(int index)                 // Get chunk by linear index
const ImageChunk& getChunkAt(int index) const     // Get chunk by linear index (const)
```

#### Conversion Methods

```cpp
void convertToColorSpace(ColorSpace targetSpace)  // Convert all chunks to target color space
Image toImage() const                             // Reconstruct Image from chunks
```

## Usage Examples

### Basic Image Loading and Manipulation

```cpp
#include "utils/image_lib.hpp"

// Load an image
Image img;
if (img.read_from_file("image.jpg")) {
    std::cout << "Image loaded: " << img.getColumns() << "x" << img.getRows() << std::endl;
    
    // Access a pixel
    Pixel pixel = img.getPixel(0, 0);
    std::cout << "First pixel RGB: " << (int)pixel.getR() << ", " 
              << (int)pixel.getG() << ", " << (int)pixel.getB() << std::endl;
    
    // Convert to YCbCr
    pixel.convertToYCbCr();
    std::cout << "First pixel YCbCr: " << (int)pixel.getY() << ", " 
              << (int)pixel.getCb() << ", " << (int)pixel.getCr() << std::endl;
}
```

### Image Chunking

```cpp
// Create chunked image with 8x8 blocks
ChunkedImage chunkedImg(img, 8);

std::cout << "Chunked into " << chunkedImg.getTotalChunks() << " chunks" << std::endl;
std::cout << "Chunk grid: " << chunkedImg.getChunkColumns() << "x" << chunkedImg.getChunkRows() << std::endl;

// Convert all chunks to YCbCr
chunkedImg.convertToColorSpace(ColorSpace::YCbCr);

// Access individual chunks
for (int i = 0; i < chunkedImg.getTotalChunks(); i++) {
    ImageChunk& chunk = chunkedImg.getChunkAt(i);
    // Process chunk for JPEG compression...
}

// Reconstruct image
Image reconstructed = chunkedImg.toImage();
```

### Different Chunk Sizes

```cpp
// 4x4 chunks for different processing
ChunkedImage smallChunks(img, 4);

// 16x16 chunks for different algorithms  
ChunkedImage largeChunks(img, 16);

// Compare chunk counts
std::cout << "4x4 chunks: " << smallChunks.getTotalChunks() << std::endl;
std::cout << "8x8 chunks: " << chunkedImg.getTotalChunks() << std::endl;
std::cout << "16x16 chunks: " << largeChunks.getTotalChunks() << std::endl;
```

## Error Handling

The library uses C++ exceptions for error handling:

- **Runtime Errors**: Thrown when accessing pixel values in the wrong color space
- **File I/O Errors**: `read_from_file()` returns `false` if image loading fails
- **Bounds Checking**: Pixel access methods include bounds checking

## Performance Considerations

- **Memory Usage**: Images are stored as 2D vectors, which may not be the most cache-efficient layout
- **Color Space Conversion**: Conversion is done pixel-by-pixel; consider batch processing for large images
- **Chunking Overhead**: Creating `ChunkedImage` involves copying all pixel data
- **File I/O**: Uses stb_image for loading, which is efficient but loads entire image into memory

## JPEG Compression Integration

This library is designed to work seamlessly with JPEG compression:

1. **Load Image**: Use `Image::read_from_file()`
2. **Create Chunks**: Use `ChunkedImage` with 8×8 chunks (JPEG standard)
3. **Convert to YCbCr**: Use `convertToColorSpace(ColorSpace::YCbCr)`
4. **Process Chunks**: Apply DCT, quantization, and encoding to individual chunks
5. **Reconstruct**: Use `toImage()` to rebuild the image after processing

## Thread Safety

The library is **not thread-safe**. Multiple threads should not access the same `Image`, `ChunkedImage`, or `ImageChunk` objects concurrently without external synchronization.

## Future Enhancements

Potential improvements for future versions:
- Support for additional color spaces (HSV, LAB, etc.)
- More efficient memory layouts (row-major, column-major)
- Thread-safe operations
- GPU acceleration support
- Additional image formats and codecs
- Memory-mapped file I/O for large images
