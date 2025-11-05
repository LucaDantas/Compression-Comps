#ifndef HAAR_TRANSFORM_HPP
#define HAAR_TRANSFORM_HPP

#include <cmath>
#include "../utils/image_lib.hpp"

// Example implementation: DCT Transform
class HaarTransform : public Transform {
public:
    // Constructor specifies transform space
    HaarTransform()
        : Transform(TransformSpace::Haar) {}

    // applies one haar transformation on the first n values
    void applyHaar1DIteration(std::vector<int>& data, int n) {
        assert(n % 2 == 0 && n <= (int)data.size());

        std::vector<int> result(n);
        for (int i = 0; i < n/2; i++) {
            result[i] = data[2*i] + data[2*i + 1];
            result[i + n/2] = data[2*i] - data[2*i + 1];
        }
        for(int i = 0; i < n; i++)
            data[i] = result[i];
    }

    // applies the inverse transform to the first n values 
    // (first half is average, second half is detail coeff)
    void inverseHaar1DIteration(std::vector<int>& data, int n) {
        assert(n % 2 == 0 && n <= (int)data.size());

        std::vector<int> result(n);
        for (int i = 0; i < n/2; i++) {
            result[2*i] = (data[i] + data[i + n/2]) / 2;
            result[2*i+1] = (data[i] - data[i + n/2]) / 2;
        }
        for(int i = 0; i < n; i++)
            data[i] = result[i];
    }

    void nonStandardDecomposition(Chunk& outputChunk) {
        for(int ch = 0; ch < 3; ch++) {
            int n = (int)outputChunk.getChunkSize();
            std::vector<std::vector<int>> result(n, std::vector<int>(n));
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    result[i][j] = outputChunk[ch][i][j];

            for(int sz = n; sz > 1; sz /= 2) {
                // apply a transformation to the rows
                for(int row = 0; row < sz; row++)
                    applyHaar1DIteration(result[row], sz);
                
                // apply a transformation to the columns
                for(int col = 0; col < sz; col++) {
                    std::vector<int> data(sz);
                    for(int i = 0; i < sz; i++)
                        data[i] = result[i][col];
                    applyHaar1DIteration(data, sz);
                    for(int i = 0; i < sz; i++)
                        result[i][col] = data[i];
                }
            }

            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    outputChunk[ch][i][j] = result[i][j];
        }
    }

    void inverseNonStandardDecomposition(Chunk& outputChunk) {
        for(int ch = 0; ch < 3; ch++) {
            int n = (int)outputChunk.getChunkSize();
            std::vector<std::vector<int>> result(n, std::vector<int>(n));
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    result[i][j] = outputChunk[ch][i][j];

            for(int sz = 2; sz <= n; sz *= 2) {
                // in the inverse first apply to column and then to row
                for(int col = 0; col < sz; col++) {
                    std::vector<int> data(sz);
                    for(int i = 0; i < sz; i++)
                        data[i] = result[i][col];
                    inverseHaar1DIteration(data, sz);
                    for(int i = 0; i < sz; i++)
                        result[i][col] = data[i];
                }

                // inverse row
                for(int row = 0; row < sz; row++)
                    inverseHaar1DIteration(result[row], sz);
            }
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    outputChunk[ch][i][j] = result[i][j];
        }
    }
    
    std::vector<std::vector<int>> getQuantizationMatrix(int size) {
        // size must be a power of 2
        std::vector<std::vector<int>> quantizationMatrix(size, std::vector<int>(size, 1));
        
        for (int sz = 1; sz <= size; sz <<= 1)
            for (int i = 0; i < sz; i++)
                for (int j = 0; j < sz; j++)
                    quantizationMatrix[i][j] <<= 1;

        return quantizationMatrix;
    }

    void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) {
        outputChunk = inputChunk;
        nonStandardDecomposition(outputChunk);
    }
    
    void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) {
        outputChunk = encodedChunk;
        inverseNonStandardDecomposition(outputChunk);
    }
};

#endif // HAAR_TRANSFORM_HPP

