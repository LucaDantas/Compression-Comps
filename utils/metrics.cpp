#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <filesystem>
#include "image_lib.hpp" 

namespace metrics {

// MSE 3 channels
inline double MSE(const Image& a, const Image& b) {
    if (a.getRows() != b.getRows() || a.getColumns() != b.getColumns()) {
        throw std::runtime_error("MSE: image sizes differ");
    }
    const long long rows = a.getRows();
    const long long cols = a.getColumns();
    const long long N = rows * cols;

    long double acc = 0.0L;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Pixel& pa = a.getPixel(r, c);
            const Pixel& pb = b.getPixel(r, c);
            for (int ch = 0; ch < 3; ++ch) {
                const long long d = static_cast<long long>(pa[ch]) - static_cast<long long>(pb[ch]);
                acc += static_cast<long double>(d) * static_cast<long double>(d);
            }
        }
    }
    return static_cast<double>(acc / (3.0L * N));
}

// Per-channel MSE (R,G,B)
void MSEChannels(const Image& a, const Image& b, double out[3]) {
    if (a.getRows() != b.getRows() || a.getColumns() != b.getColumns()) {
        throw std::runtime_error("MSEChannels: image sizes differ");
    }
    const long long rows = a.getRows();
    const long long cols = a.getColumns();
    const long long N = rows * cols;

    long double acc[3] = {0.0L, 0.0L, 0.0L};
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Pixel& pa = a.getPixel(r, c);
            const Pixel& pb = b.getPixel(r, c);
            for (int ch = 0; ch < 3; ++ch) {
                const long long d = static_cast<long long>(pa[ch]) - static_cast<long long>(pb[ch]);
                acc[ch] += static_cast<long double>(d) * static_cast<long double>(d);
            }
        }
    }
    for (int ch = 0; ch < 3; ++ch) out[ch] = static_cast<double>(acc[ch] / N);
}

// PSNR
inline double PSNRFromMSE(double mse, double maxVal = 255.0) {
    if (mse <= 0.0) return std::numeric_limits<double>::infinity();
    return 10.0 * std::log10((maxVal * maxVal) / mse);
}
inline double PSNR(const Image& a, const Image& b, double maxVal = 255.0) {
    return PSNRFromMSE(MSE(a, b), maxVal);
}

// Bits/Pixel from a file path
inline double BitsPerPixelFromFile(const std::string& filePath, int rows, int cols) {
    const auto bytes = std::filesystem::file_size(filePath);
    const long long pixels = static_cast<long long>(rows) * cols;
    return static_cast<double>(bytes) * 8.0 / static_cast<double>(pixels);
}

// Bits/Pixel from raw byte count
inline double BitsPerPixelFromBytes(std::size_t bytes, int rows, int cols) {
    const long long pixels = static_cast<long long>(rows) * cols;
    return static_cast<double>(bytes) * 8.0 / static_cast<double>(pixels);
}


} // namespace metrics
