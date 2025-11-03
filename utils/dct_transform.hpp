#ifndef DCT_TRANSFORM_HPP
#define DCT_TRANSFORM_HPP

#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include "image_lib.hpp"

// Implementation of DCT Transform using the FFT algorithm.
class DCTTransform : public Transform {
private:
    const double PI = std::acos(-1.0);
    const double RT2 = std::sqrt(2);
public:
    // Constructor specifies DCT transform space
    DCTTransform() : Transform(TransformSpace::DCT) {}

    // Reverse the bits of a number, up to the maximum range numb_bits
    int reverse(int num, int num_bits) {
        int res = 0;
        for (int i = 0; i < num_bits; i++) {
            if (num & (1 << i))
                res |= 1 << (num_bits - 1 - i);
        }
        return res;
    }
    
    // Apply the 1D DFT to a vector using the Fast Fourier Transform (FFT) algorithm. By using the 
    // even/odd parity of roots of unity and bit reversal, this allows for a fast in-place divide and
    // conquer implementation. This is inspired by the cp-algorithms page as this was the most efficient
    // and explicit design I found.
    void FFT(std::vector<std::complex<double>>& data, bool invert) {
        // If n=1 we are done
        int n = data.size();
        if (n==1)
            return;
        // Find the number of bits for n - this is needed for bit reversal
        int num_bits = 0;
        while (1 << num_bits < n) {
            num_bits++;
        }

        // Sort the indices by reverse bit order. This effectively does beforehand
        // the result of grouping even and odd indices then merging each time.
        for (int i = 0; i < n; i++) {
            if (i < reverse(i, num_bits))
                std::swap(data[i], data[reverse(i, num_bits)]);
        }

        // The first loop is the number of iterations needed, log(N).
        for (int k = 2; k <= n; k <<= 1) {
            // Find the kth root of unity
            double theta = 2 * PI / k * (invert ? -1 : 1);
            std::complex<double> w(cos(theta), sin(theta));
            // Perform the FFT on each length k section of data. Note that the symmetry
            // u + v, u - v is called a butterfly!
            for (int i = 0; i < n; i += k) {
                std::complex<double> w_n(1);
                for (int j = 0; j < k / 2; j++) {
                    std::complex<double> u = data[i+j], v = data[i+j+k/2] * w_n;
                    data[i+j] = u + v;
                    data[i+j+k/2] = u - v;
                    w_n *= w;
                }
            }
        }

        // Scale the data by sqrt(N) so that forward and inverse are consistent.
        for (std::complex<double>& x : data)
            x /= std::sqrt(n);
    }

    void ForwardFFT(std::vector<std::complex<double>>& data) {
        FFT(data, 0);
    }

    void InverseFFT(std::vector<std::complex<double>>& data) {
        FFT(data, 1);
    }

    void ForwardFCT(std::vector<double>& data) {
        int n = data.size();
        std::vector<std::complex<double>> mirrored_data(2*n);
        for (int i = 0; i < n; i++) {
            mirrored_data[i] = std::complex<double>(data[i]);
            mirrored_data[2*n-1-i] = mirrored_data[i];
        }
        ForwardFFT(mirrored_data);
        double theta = -PI / (2*n);
        double scale = 2 * RT2 / (std::sqrt(n));
        std::complex<double> w(std::cos(theta), -std::sin(theta));
        std::complex<double> w_n(1);
        for (int i = 0; i < n; i++) {
            data[i] = (scale * ((i == 0) ? 1 / RT2 : 1) * w_n * mirrored_data[i]).real();
            w_n *= w;
        }
    }

    void InverseFCT(std::vector<double>& data) {
        int n = data.size();
        double theta = PI / (2*n);
        double scale = std::sqrt(n) / (2 * RT2);
        std::vector<std::complex<double>> mirrored_data(2*n);
        for (int i = 0; i < n; i++) {
            data[i] *= scale * ((i == 0) ? RT2: 1);
            mirrored_data[i] = std::complex<double>(data[i]);
            if (i > 0) mirrored_data[2*n - i] = std::complex<double>(-data[i]);
        }
        mirrored_data[n] = 0;
        std::complex<double> w(std::cos(theta), -std::sin(theta));
        std::complex<double> w_n(1);
        for (int i = 0; i < 2*n; i++) {
            mirrored_data[i] *= w_n;
            w_n *= w;
        }
        InverseFFT(mirrored_data);
        for (int i = 0; i < n; i++) {
            data[i] = mirrored_data[i].real();
        }
    }


    // Implement encodeChunk for DFT: O(N^2logN) implementation
    void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) {
        int n = inputChunk.getChunkSize();

        // Simple example: copy all channel data
        for (int ch = 0; ch < 3; ch++) {
            std::vector<std::vector<double>> result(n, std::vector<double>(n));
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    result[i][j] = (double)inputChunk[ch][i][j] - 128;
                }
            }

            for (int row = 0; row < n; row++)
                ForwardFCT(result[row]);


            for (int col = 0; col < n; col++) {
                std::vector<double> temp_col(n);
                for (int i = 0; i < n; i++)
                    temp_col[i] = result[i][col];
                ForwardFCT(temp_col);
                for (int i = 0; i < n; i++)
                    result[i][col] = temp_col[i];
            }

            for(int i = 0; i < n; i++) {
                for(int j = 0; j < n; j++) {
                    outputChunk[ch][i][j] = std::round(result[i][j]);
                }
            }
        }
    }
    
    // Implement decodeChunk for DCT: n^4 implementation
    void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) {
        int n = encodedChunk.getChunkSize();

        // Simple example: copy all channel data
        for (int ch = 0; ch < 3; ch++) {
            std::vector<std::vector<double>> result(n, std::vector<double>(n));
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    result[i][j] = (double)encodedChunk[ch][i][j];
                }
            }

            for (int row = 0; row < n; row++)
                InverseFCT(result[row]);


            for (int col = 0; col < n; col++) {
                std::vector<double> temp_col(n);
                for (int i = 0; i < n; i++)
                    temp_col[i] = result[i][col];
                InverseFCT(temp_col);
                for (int i = 0; i < n; i++)
                    result[i][col] = temp_col[i];
            }

            for(int i = 0; i < n; i++) {
                for(int j = 0; j < n; j++) {
                    outputChunk[ch][i][j] = std::round(result[i][j]) + 128;
                }
            }
        }
    }
};

#endif // DCT_TRANSFORM_HPP
