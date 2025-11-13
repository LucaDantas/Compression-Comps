#ifndef DFT_TRANSFORM_HPP
#define DFT_TRANSFORM_HPP

#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include "../utils/transform.hpp"

// Implementation of DFT Transform using the FFT algorithm.
class DFTTransform : public Transform {
private:
    const double PI = std::acos(-1.0);
    const double RT2 = std::sqrt(2);
public:
    // Constructor specifies DCT transform space
    DFTTransform() : Transform(TransformSpace::DFT) {}

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

    // Formal forward and inverse functions so invert parameter doesn't have to be passed.
    void ForwardFFT(std::vector<std::complex<double>>& data) {
        FFT(data, 0);
    }

    void InverseFFT(std::vector<std::complex<double>>& data) {
        FFT(data, 1);
    }

    // Implement encodeChunk for DFT: O(N^2logN) implementation
    void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) {
        int n = inputChunk.getChunkSize();

        std::vector<std::vector<std::complex<double>>> result(n, std::vector<std::complex<double>>(n));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                result[i][j] = (double)inputChunk[0][i][j] - 128;
            }
        }

        for (int row = 0; row < n; row++)
            ForwardFFT(result[row]);


        for (int col = 0; col < n; col++) {
            std::vector<std::complex<double>> temp_col(n);
            for (int i = 0; i < n; i++)
                temp_col[i] = result[i][col];
            ForwardFFT(temp_col);
            for (int i = 0; i < n; i++)
                result[i][col] = temp_col[i];
        }

        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                outputChunk[0][i][j] = std::round(result[i][j].real());
                outputChunk[1][i][j] = std::round(result[i][j].imag());
            }
        }
    }
    
    // Implement decodeChunk for DCT: n^4 implementation
    void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) {
        int n = encodedChunk.getChunkSize();

        // Simple example: copy all channel data
        std::vector<std::vector<std::complex<double>>> result(n, std::vector<std::complex<double>>(n));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                result[i][j] = std::complex<double>(encodedChunk[0][i][j], encodedChunk[1][i][j]);
            }
        }

        for (int row = 0; row < n; row++)
            InverseFFT(result[row]);


        for (int col = 0; col < n; col++) {
            std::vector<std::complex<double>> temp_col(n);
            for (int i = 0; i < n; i++)
                temp_col[i] = result[i][col];
            InverseFFT(temp_col);
            for (int i = 0; i < n; i++)
                result[i][col] = temp_col[i];
        }

        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                outputChunk[0][i][j] = std::round(result[i][j].real()) + 128;
            }
        }
    }
};

#endif // DFT_TRANSFORM_HPP
