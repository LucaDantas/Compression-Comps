#ifndef DFT_TRANSFORM_HPP
#define DFT_TRANSFORM_HPP

#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include "image_lib.hpp"

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

    int zip(int r, int c) {
        int res = 0;
        if (r < 0) {
            res = 1 << 21;
            r = -r;
        }
        res |= (r << 11);
        if (c < 0) {
            res |= 1 << 10;
            c = -c;
        }
        res |= c;

        return res;
    }

    std::pair<int, int> unzip(int code) {
        int r = code >> 11;
        if (r >> 10 == 1) {
            r = -(r & 0x3FF);
        }
        int c = code & 0x7FF;
        if (c >> 10 == 1) {
            c = -(c & 0x3FF);
        }
        return {r, c};
    }

    // Implement encodeChunk for DFT: O(N^2logN) implementation
    void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) {
        int n = inputChunk.getChunkSize();

        for (int ch = 0; ch < 3; ch++) {
            std::vector<std::vector<std::complex<double>>> result(n, std::vector<std::complex<double>>(n));
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    result[i][j] = (double)inputChunk[ch][i][j] - 128;
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
                    outputChunk[ch][i][j] = zip((int)std::round(result[i][j].real()), (int)std::round(result[i][j].imag()));
                }
            }
        }
    }
    
    // Implement decodeChunk for DCT: n^4 implementation
    void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) {
        int n = encodedChunk.getChunkSize();

        for (int ch = 0; ch < 3; ch++) {
            std::vector<std::vector<std::complex<double>>> result(n, std::vector<std::complex<double>>(n));
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    std::pair<int, int> vals = unzip(encodedChunk[ch][i][j]);
                    result[i][j] = std::complex<double>(vals.first, vals.second);
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
                    outputChunk[ch][i][j] = std::round(result[i][j].real()) + 128;
                }
            }
        }
    }

    void quantizeChunk(const Chunk& inputChunk, Chunk& outputChunk, double scale) {
		
		int size = inputChunk.getChunkSize();
		int result;
		int realValue;
        int complexValue;
		int matrixValue;
		std::pair<int, int> vals;

		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
                    vals = unzip(inputChunk[ch][u][v]);
					realValue = std::round(vals.first / scale);
                    complexValue = std::round(vals.second / scale);
					result = zip(realValue, complexValue);
					outputChunk[ch][u][v] = result;
				}
			}
		}
	}

	void dequantizeChunk(const Chunk& encodedChunk, Chunk& outputChunk, double scale) {
		
		int size = encodedChunk.getChunkSize();
		int result;
		int realValue;
        int complexValue;
		int matrixValue;
		std::pair<int, int> vals;

		for (int ch = 0; ch < 3; ch++) {
			for (int u = 0; u < size; u++) {
				for (int v = 0; v < size; v++) {
                    vals = unzip(encodedChunk[ch][u][v]);
					realValue = vals.first * scale;
                    complexValue = vals.second * scale;
					result = zip(realValue, complexValue);
					outputChunk[ch][u][v] = result;
				}
			}
		}
	}
};

#endif // DFT_TRANSFORM_HPP
