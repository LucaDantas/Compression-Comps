# Compression Pipeline Documentation

## Overview

This project implements an image compression pipeline that applies various mathematical transforms to images, followed by entropy coding and Huffman encoding to achieve lossy compression. The pipeline supports multiple transform types (DCT, DFT, Haar, S+P) and provides comprehensive utilities for image processing, encoding, and analysis.

## Pipeline Architecture

The compression pipeline follows a standard transform coding approach with the following stages:

### Encoding Pipeline
1. **Image Loading**: Load PNG images in RGB format
2. **Color Space Conversion**: Convert RGB to YCbCr color space
3. **Chunking**: Divide image into fixed-size chunks (typically 8x8 for DCT/DFT, full image for Haar/S+P)
4. **Transform Application**: Apply selected transform (DCT, DFT, Haar, or S+P) to each chunk
5. **Quantization**: Apply quantization with a specified scale factor to reduce precision
6. **Entropy Coding**: Apply DPCM (Differential Pulse Code Modulation) and RLE (Run-Length Encoding)
7. **Huffman Encoding**: Apply Huffman coding to further compress the data
8. **Binary Output**: Write compressed data to binary file

### Decoding Pipeline
The decoding pipeline reverses the encoding process:
1. **Binary Input**: Read compressed data from binary file
2. **Huffman Decoding**: Decode Huffman-encoded data
3. **Entropy Decoding**: Reverse RLE and DPCM operations
4. **Inverse Quantization**: Restore quantized values
5. **Inverse Transform**: Apply inverse transform to reconstruct image chunks
6. **Color Space Conversion**: Convert YCbCr back to RGB
7. **Image Reconstruction**: Reconstruct and save the decoded image

## Main Pipeline Components

### `pipeline_controller.cpp`
A testing and visualization tool that processes a single image through the compression pipeline. It provides detailed analytics including:
- Entropy calculations at each stage
- Encoded image visualization
- Difference image analysis
- Pixel-level statistics and comparisons

### `pipeline_data_collection.cpp`
The main experimental pipeline that processes images and outputs compression metrics. It measures:
- Compression ratios
- Entropy values at different stages
- Image quality metrics (MSE, PSNR)
- Encoding and decoding times

## Utility Modules (`utils/`)

### `image_lib.hpp`
Core image data structures and operations.

#### Classes:
- **`Pixel`**: Represents a single pixel with 3 color channels (RGB or YCbCr). Supports color space conversion operations.

- **`Image`**: Main image container class that stores pixel data in a 2D grid. Provides:
  - Image loading from PNG files
  - Color space conversions (RGB ↔ YCbCr ↔ Grayscale)
  - Entropy calculation
  - Image saving functionality
  - Channel extraction and visualization

- **`Chunk`**: Represents a square block of image data (typically 8x8 pixels). Stores data in channel-first format: `channels[channel][row][col]`.

- **`ChunkedImage`**: Container for managing images divided into chunks. Maintains:
  - Grid of chunks
  - Transform space metadata (Raw, DCT, DWT, Haar, SP, DFT)
  - Color space metadata
  - Chunk size and dimensions

#### Enums:
- **`TransformSpace`**: Indicates the current transform domain (Raw, DCT, DWT, Haar, SP, DFT)
- **`ColorSpace`**: Indicates the color representation (RGB, YCbCr, Grayscale)

### `transform.hpp`
Abstract base class for image transforms.

#### Classes:
- **`Transform`**: Abstract base class that defines the interface for all transforms. Provides:
  - Forward transform application (`applyTransform`)
  - Inverse transform application (`applyInverseTransform`)
  - Quantization (`applyQuantization`)
  - Inverse quantization (`applyInverseQuantization`)
  - Transform space identification

Derived transform classes (DCTTransform, DFTTransform, HaarTransform, SPTransform) implement specific transform algorithms.

### `entropy.hpp`
Entropy coding module that applies prediction and run-length encoding.

#### Functions:
- **`EntropyEncode`**: Main encoding function that applies entropy coding to a chunked image. Uses different strategies for DCT/DFT vs Haar/S+P transforms:
  - **DCT/DFT**: Separates DC components (encoded with DPCM) and AC components (encoded with RLE in zigzag order)
  - **Haar/S+P**: Applies DPCM to all coefficients in zigzag order

- **`EntropyDecode`**: Main decoding function that reverses entropy coding operations.

#### Data Structures:
- **`EntropyEncoded`**: Structure containing DC components (DPCM-encoded) and AC components (RLE-encoded pairs)

### `dpcm.hpp`
Differential Pulse Code Modulation (DPCM) implementation for prediction coding.

#### Functions:
- **`dpcm::encoder`**: Encodes an array by predicting each value based on previous values using linear prediction. Returns differences between actual and predicted values.

- **`dpcm::decoder`**: Decodes DPCM-encoded data by reconstructing values from differences and predictions.

#### Helper Functions:
- **`zigzagFlattenArray`**: Converts a 2D array to 1D using zigzag scanning pattern (used for DCT coefficients)
- **`unflattenArray`**: Reverses zigzag flattening to reconstruct 2D array
- **`linearPredictor`**: Implements linear prediction using least squares regression

### `rle.hpp`
Run-Length Encoding (RLE) implementation for compressing sequences of zeros.

#### Functions:
- **`rle::encoder`**: Encodes an array by representing runs of zeros as (count, value) pairs. Limits run length to 15 zeros per pair.

- **`rle::decoder`**: Decodes RLE-encoded data by expanding (count, value) pairs back into full sequences.

### `huffman.hpp`
Huffman coding implementation for lossless compression.

#### Classes:
- **`HuffmanTree`**: Binary tree structure for Huffman encoding. Provides:
  - Tree construction from frequency maps
  - Tree serialization/deserialization
  - Encoding map generation
  - Decoding from binary strings

- **`HuffmanEncoder`**: Encodes integer vectors using Huffman coding. Outputs:
  - Serialized tree structure
  - Encoded data as bit-packed integers

- **`HuffmanDecoder`**: Decodes Huffman-encoded data by reconstructing the tree and decoding the bit stream.

### `binary_io.hpp`
File I/O utilities for reading and writing binary data.

#### Functions:
- **`writeVectorToFile`**: Writes a vector of integers to a binary file
- **`readVectorFromFile`**: Reads a vector of integers from a binary file

### `metrics.hpp`
Image quality and compression metrics.

#### Functions:
- **`metrics::MSE`**: Calculates Mean Squared Error between two images
- **`metrics::MSEChannels`**: Calculates per-channel MSE values
- **`metrics::PSNR`**: Calculates Peak Signal-to-Noise Ratio from MSE
- **`metrics::PSNRFromMSE`**: Converts MSE to PSNR
- **`metrics::BitsPerPixelFromFile`**: Calculates bits per pixel from file size
- **`metrics::BitsPerPixelFromBytes`**: Calculates bits per pixel from byte count

### `timer.hpp`
Performance measurement utilities.

#### Classes:
- **`cscomps::util::Timer`**: Stopwatch-style timer for measuring elapsed time in milliseconds
- **`cscomps::util::ScopedTimer`**: RAII-style timer that automatically records elapsed time to a reference variable

## Transform Implementations (`transforms/`)

The pipeline supports four transform types:

1. **DCT (Discrete Cosine Transform)**: Uses FFT-based implementation, typically applied to 8x8 blocks
2. **DFT (Discrete Fourier Transform)**: Frequency domain transform, typically applied to 8x8 blocks
3. **Haar (Haar Wavelet Transform)**: Wavelet transform, typically applied to full image
4. **S+P (Smooth + Predict Transform)**: Wavelet-based transform, typically applied to full image

Each transform class inherits from `Transform` and implements:
- Forward transform (encoding)
- Inverse transform (decoding)
- Quantization matrix generation
- Transform-specific quantization/dequantization

## Data Flow

```
Original Image (PNG)
    ↓
Image Object (RGB)
    ↓
Color Space Conversion (YCbCr)
    ↓
ChunkedImage (chunks of fixed size)
    ↓
Transform Application (DCT/DFT/Haar/S+P)
    ↓
Quantization (with scale factor)
    ↓
Entropy Encoding (DPCM + RLE)
    ↓
Huffman Encoding
    ↓
Binary File Output
```

The decoding process reverses this flow, with each stage performing the inverse operation.

## Key Design Features

1. **Modular Architecture**: Each stage of the pipeline is implemented as a separate, reusable module
2. **Transform Abstraction**: All transforms inherit from a common base class, allowing easy addition of new transforms
3. **Chunked Processing**: Images are processed in chunks to enable block-based transforms and efficient memory usage
4. **Metadata Preservation**: Transform space and color space information is maintained throughout the pipeline
5. **Comprehensive Metrics**: Built-in support for measuring compression ratios, image quality, and performance

