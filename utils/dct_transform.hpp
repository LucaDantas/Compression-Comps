#include <cmath>
#include "image_lib.hpp"

// Example implementation: DCT Transform
class DCTTransform : public Transform {
public:
    // Constructor specifies DCT transform space
    DCTTransform() : Transform(DataSpace::DCT) {}
    
    // Implement encodeChunk - simple example that just copies data
    TransformChunk encodeChunk(const TransformChunk& inputChunk) {
        TransformChunk result(inputChunk.getChunkSize(), DataSpace::DCT);
        double sum, cu, cv;
        int pixel;
        int n = inputChunk.getChunkSize();

        // Simple example: copy all channel data
        for (int ch = 0; ch < 3; ch++) {
            const std::vector<std::vector<int>>& inputChannel = inputChunk[ch];
            std::vector<std::vector<int>>& resultChannel = result[ch];
            for (int u = 0; u < n; u++) {
                for (int v = 0; v < n; v++) {
                    sum = 0;
                    if (u == 0) cu = 1 / std::sqrt(2); else cu = 1;
                    if (v == 0) cv = 1 / std::sqrt(2); else cv = 1;
                    for (int i = 0; i < n; i++) {
                        for (int j = 0; j < n; j++) {
                            pixel = inputChannel[i][j] - 128;
                            sum += pixel * std::cos((M_PI * (2*i + 1) * u) / (2*n)) * std::cos((M_PI * (2*j + 1) * v) / (2*n));
                        }
                    }
                    resultChannel[u][v] = std::round((static_cast<double>(2) / n) * cu * cv * sum);
                }
            }
        }
        
        return result;
    }
    
    // Implement decodeChunk - simple example that just copies data back
    TransformChunk decodeChunk(const TransformChunk& encodedChunk) {
        TransformChunk result(encodedChunk.getChunkSize(), DataSpace::RGB);
        double sum, cu, cv;
        int pixel;
        int n = encodedChunk.getChunkSize();

        // Simple example: copy all channel data back
        for (int ch = 0; ch < 3; ch++) {
            const std::vector<std::vector<int>>& encodedChannel = encodedChunk[ch];
            std::vector<std::vector<int>>& resultChannel = result[ch];
            
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    sum = 0;
                    resultChannel[i][j] = encodedChannel[i][j];
                    for (int u = 0; u < n; u++) {
                        for (int v = 0; v < n; v++) {
                            if (u == 0) cu = 1 / std::sqrt(2); else cu = 1;
                            if (v == 0) cv = 1 / std::sqrt(2); else cv = 1;
                            pixel = encodedChannel[u][v];
                            sum += pixel * cu * cv * std::cos((M_PI * (2*i + 1) * u) / (2*n)) * std::cos((M_PI * (2*j + 1) * v) / (2*n));
                        }
                    }
                    resultChannel[i][j] = std::round((2 / n) * sum) + 128;
                }
            }
        }
        
        return result;
    }
};
