# Image Library Documentation

## Overview

The image library provides a comprehensive framework for image processing with support for different color spaces, transform spaces, and chunked image operations. It includes classes for pixel manipulation, image representation, chunked image processing, and abstract transform interfaces.

## Enums

### TransformSpace
Defines the mathematical transform applied to image data.

**Values:**
- `Raw`: Raw image data (no transform applied)
- `DCT`: Discrete Cosine Transform
- `DWT`: Discrete Wavelet Transform
- `SP`: S+P Transform

### ColorSpace
Defines the color representation format.

**Values:**
- `RGB`: Red-Green-Blue color space
- `YCbCr`: Luminance-Chrominance color space

## Helper Functions

### colorSpaceToString(ColorSpace colorSpace)
Converts ColorSpace enum to string representation.

**Parameters:**
- `colorSpace`: The color space enum value

**Returns:** String representation of the color space

### transformSpaceToString(TransformSpace transformSpace)
Converts TransformSpace enum to string representation.

**Parameters:**
- `transformSpace`: The transform space enum value

**Returns:** String representation of the transform space

## Classes

### Pixel

Represents a single pixel with three color channels. Provides direct access to channel values and color space conversion capabilities.

**Main Functionality:**
- Stores RGB or YCbCr pixel data
- Provides bounds-checked channel access via operator[]
- Converts between RGB and YCbCr color spaces

**Key Methods:**
- `operator[](int idx)`: Direct access to channel values with bounds checking
- `convertToRGB()`: Converts YCbCr pixel to RGB
- `convertToYCbCr()`: Converts RGB pixel to YCbCr

### Image

Represents a complete image with pixel data, transform space, and color space information. Handles image loading from files and color space conversions.

**Main Functionality:**
- Loads images from files using STB library
- Manages pixel data in a 2D grid
- Tracks transform space and color space
- Provides color space conversion for entire image

**Key Methods:**
- `Image(const std::string& filename)`: Loads image from file
- `convertToRGB()`: Converts entire image from YCbCr to RGB
- `convertToYCbCr()`: Converts entire image from RGB to YCbCr
- `printInfo()`: Displays image information including dimensions and spaces

**Parameters:**
- `rows`, `columns`: Image dimensions
- `transformSpace`: Current transform state
- `colorSpace`: Current color representation

### Chunk

Represents a square block of pixel data organized by channels. Used for efficient processing of image regions.

**Main Functionality:**
- Stores 3-channel data in channel-first format
- Provides bounds-checked channel access
- Supports variable chunk sizes

**Key Methods:**
- `operator[](int idx)`: Access to channel data with bounds checking
- `getChunkSize()`: Returns the size of the square chunk

**Parameters:**
- `chunkSize`: Size of the square chunk (e.g., 8x8)

### ChunkedImage

Manages a collection of chunks representing a complete image. Handles chunk organization, padding, and transform space tracking.

**Main Functionality:**
- Organizes image data into chunks for efficient processing
- Handles padding for images that don't divide evenly into chunks
- Tracks transform space and color space across all chunks
- Creates fresh copies for transform results

**Key Methods:**
- `ChunkedImage(const Image& image, int chunkSize)`: Creates chunked image from regular image
- `ChunkedImage(int originalRows, int originalColumns, TransformSpace transformSpace, ColorSpace colorSpace, int chunkSize)`: Creates empty chunked image
- `getChunkAt(int index)`: Retrieves chunk by linear index
- `createFreshCopyForTransformResult(TransformSpace resultTransformSpace)`: Creates new chunked image for transform results
- `printInfo()`: Displays chunked image information

**Parameters:**
- `originalRows`, `originalColumns`: Original image dimensions
- `chunkRows`, `chunkColumns`: Number of chunks in each dimension
- `chunkSize`: Size of each square chunk
- `transformSpace`: Current transform state
- `colorSpace`: Current color representation

### Transform

Abstract base class for image transforms. Defines the interface for encoding and decoding operations on chunked images.

**Main Functionality:**
- Provides abstract interface for transform implementations
- Manages transform space validation
- Handles chunk-by-chunk processing
- Supports both forward and inverse transforms

**Key Methods:**
- `applyTransform(const ChunkedImage& chunkedImage)`: Applies forward transform to chunked image
- `applyInverseTransform(const ChunkedImage& chunkedImage)`: Applies inverse transform to chunked image
- `encodeChunk(const Chunk& inputChunk, Chunk& outputChunk)`: Pure virtual method for chunk encoding
- `decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk)`: Pure virtual method for chunk decoding

**Parameters:**
- `transformSpace`: The transform space this transform operates in

**Protected Methods:**
- `encodeChunk()`: Must be implemented by derived classes for encoding
- `decodeChunk()`: Must be implemented by derived classes for decoding

## Usage Example

```cpp
// Load image
Image img("image.png");

// Create chunked image
ChunkedImage chunkedImg(img, 8);

// Apply transform (example with DCT)
DCTTransform dctTransform;
ChunkedImage encoded = dctTransform.applyTransform(chunkedImg);

// Apply inverse transform
ChunkedImage decoded = dctTransform.applyInverseTransform(encoded);
```
