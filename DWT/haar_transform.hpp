#ifndef HAAR_TRANSFORM_HPP
#define HAAR_TRANSFORM_HPP

#include <cmath>
#include "../utils/image_lib.hpp"

// Example implementation: DCT Transform
class HaarTransform : public Transform {
private:
    // decreasing this number decreases the precision of the transform
    // but increases the entropy reduction (lossy)
    double transformScale;
    double detailScale;
	
	std::vector<std::vector<int>> getQuantizationMatrix(int size) {
		
		std::vector<std::vector<int>> quantizationMatrix(size, std::vector<int>(size, 1));
		
		int recursions = std::log2(size);
		
		for (int r = 0; r < recursions; r++) {
		
			for (int i = 0; i < (size/std::pow(2, r)); i++) {
				for (int j = 0; j < (size/std::pow(2, r)); j++) {
					if (i >= (size/std::pow(2, r+1)) || j >= (size/std::pow(2, r+1)) ) {
						quantizationMatrix[i][j] = quantizationMatrix[i][j]*std::pow(2, r+1);
					}
				}
			}
		
		}
		
		quantizationMatrix[0][0] = quantizationMatrix[0][0]*std::pow(2, recursions);

		return quantizationMatrix;
		
	}
	
public:
    // Constructor specifies transform space
    HaarTransform(double transformScale = 0.125, double detailScale = 1)
        : Transform(TransformSpace::Haar), transformScale(transformScale), detailScale(detailScale) {}

    // applies one haar transformation on the first n values
    void applyHaar1DIteration(std::vector<double>& data, int n) {
        assert(n % 2 == 0 && n <= (int)data.size());

        std::vector<double> result(n);
        for (int i = 0; i < n/2; i++) {
            result[i] = (data[2*i] + data[2*i + 1]) / std::sqrt(2);
            result[i + n/2] = (data[2*i] - data[2*i + 1]) * detailScale / std::sqrt(2);
        }
        for(int i = 0; i < n; i++)
            data[i] = result[i];
    }

    // applies the inverse transform to the first n values 
    // (first half is average, second half is detail coeff)
    void inverseHaar1DIteration(std::vector<double>& data, int n) {
        assert(n % 2 == 0 && n <= (int)data.size());

        std::vector<double> result(n);
        for (int i = 0; i < n/2; i++) {
            result[2*i] = (data[i] + data[i + n/2] / detailScale) / std::sqrt(2);
            result[2*i+1] = (data[i] - data[i + n/2] / detailScale) / std::sqrt(2);
        }
        for(int i = 0; i < n; i++)
            data[i] = result[i];
    }

    void nonStandardDecomposition(Chunk& outputChunk) {
        for(int ch = 0; ch < 3; ch++) {
            int n = (int)outputChunk.getChunkSize();
            std::vector<std::vector<double>> result(n, std::vector<double>(n));
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    result[i][j] = outputChunk[ch][i][j] * transformScale;

            for(int sz = n; sz > 1; sz /= 2) {
                // apply a transformation to the rows
                for(int row = 0; row < sz; row++)
                    applyHaar1DIteration(result[row], sz);
                
                // apply a transformation to the columns
                for(int col = 0; col < sz; col++) {
                    std::vector<double> data(sz);
                    for(int i = 0; i < sz; i++)
                        data[i] = result[i][col];
                    applyHaar1DIteration(data, sz);
                    for(int i = 0; i < sz; i++)
                        result[i][col] = data[i];
                }
            }

            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    outputChunk[ch][i][j] = round(result[i][j]);
        }
    }

    void inverseNonStandardDecomposition(Chunk& outputChunk) {
        for(int ch = 0; ch < 3; ch++) {
            int n = (int)outputChunk.getChunkSize();
            std::vector<std::vector<double>> result(n, std::vector<double>(n));
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    result[i][j] = outputChunk[ch][i][j];

            for(int sz = 2; sz <= n; sz *= 2) {
                // in the inverse first apply to column and then to row
                for(int col = 0; col < sz; col++) {
                    std::vector<double> data(sz);
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
                    outputChunk[ch][i][j] = round(result[i][j] / transformScale);
        }
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

