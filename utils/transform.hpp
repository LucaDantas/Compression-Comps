#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "image_lib.hpp"

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
	
	virtual std::vector<std::vector<int>> getQuantizationMatrix(int size) {
        return std::vector<std::vector<int>>(size, std::vector<int>(size, 1));  
    }
	
	virtual void quantizeChunk(const Chunk& inputChunk, Chunk& outputChunk, double scale) {
		
		int size = inputChunk.getChunkSize();
		int result;
		int inputValue;
		double matrixValue;

        auto quantizationMatrix = getQuantizationMatrix(size);
		
		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
					inputValue = inputChunk[ch][u][v];
					matrixValue = quantizationMatrix[u][v] * scale;
					result = std::round(inputValue / matrixValue);
					outputChunk[ch][u][v] = result;
				}
			}
		}
	}
	virtual void dequantizeChunk(const Chunk& encodedChunk, Chunk& outputChunk, double scale) {
		
		int size = encodedChunk.getChunkSize();
		int result;
		int encodedValue;
		double matrixValue;

        auto quantizationMatrix = getQuantizationMatrix(size);
		
		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
					encodedValue = encodedChunk[ch][u][v];
					matrixValue = quantizationMatrix[u][v] * scale;
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

    // Generate visualization steps for the transform
    virtual std::vector<ChunkedImage> generateVisualizationSteps(const ChunkedImage& chunkedImage) {
        std::vector<ChunkedImage> steps;
        
        // Check that the chunkedImage is in Raw transform space
        if (chunkedImage.getTransformSpace() != TransformSpace::Raw) {
            throw std::runtime_error(
                "ChunkedImage transform space (" + transformSpaceToString(chunkedImage.getTransformSpace()) + 
                ") is not Raw. Transform can only be applied to Raw data."
            );
        }
        
        // Create a fresh ChunkedImage with same parameters but in the transform's final transform space
        ChunkedImage currentResult = chunkedImage.createFreshCopyForTransformResult(transformSpace);
        
        // Initialize with input data (copying Raw data into the result structure)
        for (int i = 0; i < chunkedImage.getTotalChunks(); i++) {
            currentResult.getChunkAt(i) = chunkedImage.getChunkAt(i);
        }
        
        // Add initial state
        steps.push_back(currentResult);
        
        // Apply encoding to each chunk and snapshot
        for (int i = 0; i < currentResult.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = currentResult.getChunkAt(i);
            encodeChunk(inputChunk, resultChunk);
            
            // Add snapshot after each chunk
            steps.push_back(currentResult);
        }
        
        return steps;
    }
	
	ChunkedImage applyQuantization(const ChunkedImage& chunkedImage, double scale) {
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(transformSpace);
		
        // Apply decoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            quantizeChunk(inputChunk, resultChunk, scale);
        }
        
        return result;

	}
	
	ChunkedImage applyInverseQuantization(const ChunkedImage& chunkedImage, double scale) {
		
        // if (chunkedImage.getChunkSize() != 8) {
        //     throw std::runtime_error(
        //         "ChunkedImage chunk size (" + std::to_string(chunkedImage.getChunkSize()) + 
        //         ") is not 8. Default quantizer currently only supports 8x8 chunks."
        //     );
        // }
		
        ChunkedImage result = chunkedImage.createFreshCopyForTransformResult(transformSpace);
		
        // Apply decoding to each chunk
        for (int i = 0; i < result.getTotalChunks(); i++) {
            const Chunk& inputChunk = chunkedImage.getChunkAt(i);
            Chunk& resultChunk = result.getChunkAt(i);
            dequantizeChunk(inputChunk, resultChunk, scale);
        }
        
        return result;

	}
    
    // Accessor methods
    TransformSpace getTransformSpace() const { return transformSpace; }
};

#endif // TRANSFORM_HPP

