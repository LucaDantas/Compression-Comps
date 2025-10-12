#include <iostream>
#include "utils/image_lib.hpp"
#include "utils/dct_transform.hpp"

int main() {
    std::cout << "Starting Transform interface test..." << std::endl;
    
    // Create a simple test image
    Image img(16, 16);
    std::cout << "Test image created: " << img.getRows() << "x" << img.getColumns() << std::endl;
    
    // Create DCT Transform
    std::cout << "Creating DCT Transform..." << std::endl;
    DCTTransform dctTransform(img, 8);
    std::cout << "DCT Transform created successfully!" << std::endl;
    std::cout << "Transform space: " << dctTransform.getTransformSpaceString() << std::endl;
    std::cout << "Chunk size: " << dctTransform.getChunkSize() << std::endl;
    std::cout << "Total chunks: " << dctTransform.getTotalChunks() << std::endl;
    
    // Apply transform (encoding only)
    std::cout << "Applying DCT transform..." << std::endl;
    TransformedChunkedImage encodedResult = dctTransform.applyTransform();
    std::cout << "DCT transform applied successfully!" << std::endl;
    
    // Apply full transform (encoding + decoding)
    std::cout << "Applying full DCT transform (encode + decode)..." << std::endl;
    TransformedChunkedImage decodedResult = dctTransform.applyFullTransform();
    std::cout << "Full DCT transform applied successfully!" << std::endl;
    
    std::cout << "Transform interface test completed successfully!" << std::endl;
    return 0;
}