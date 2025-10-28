#pragma once
#include <cstddef>
#include <string>
#include "image_lib.hpp"

namespace metrics {
double MSE(const Image& a, const Image& b);
void   MSEChannels(const Image& a, const Image& b, double out[3]);
double PSNRFromMSE(double mse, double maxVal = 255.0);
double PSNR(const Image& a, const Image& b, double maxVal = 255.0);
double BitsPerPixelFromFile(const std::string& filePath, int rows, int cols);
double BitsPerPixelFromBytes(std::size_t bytes, int rows, int cols);
double BitsPerPixelFromEntropy(double entropy_bpp);
}
