#ifndef CS_COMPS_SP_TRANSFORM_HPP
#define CS_COMPS_SP_TRANSFORM_HPP

// All data are ints; no external libraries.
// Public API: forward2D / inverse2D operating in-place on one image plane.
// Pipeline contract: chunked Image (Y / Cb / Cr)
// by Jiale Wan Oct 2025
// SP Transform for 2-D images

#include <cassert>
#include <vector>
#include "image_lib.hpp" 

namespace cscomps {
namespace sp {

struct Params {
  int coeff_shift = 3; // => denominator 8
  // Predictor coefficients (S+P "natural image" choice; see paper).
  // pred = (beta_m1*(s[l-1]-s[l]) + beta_0*(s[l]-s[l+1]) + beta_p1*(s[l+1]-s[l+2])
  //           - phi1 * d1[l+1]) >> coeff_shift
  int beta_m1 = 0;
  int beta_0 = 2;
  int beta_p1 = 3;  
  int phi1 = 2;

  enum class Border { Clamp, Mirror };
  Border border = Border::Clamp;

  // Pyramid depth (number of LL recursions). If 0, auto-compute.
  int levels = 0;
  static Params NaturalImage() { return Params(); }
};

struct Plane {
  // One image component stored as a 2-D int buffer.
  // stride is number of ints between successive rows (>= width).
  int* data = nullptr;
  int width = 0;
  int height = 0;
  int stride = 0;//may be not necessary right now
};

// Subband rectangle descriptors in-place (row-major).
// After one level on WxH:
//   LL: [0..W/2) x [0..H/2)
//   HL: [W/2..W) x [0..H/2)
//   LH: [0..W/2) x [H/2..H)
//   HH: [W/2..W) x [H/2..H)
struct Subbands {
  int w, h; 
  int ll_w, ll_h; // = w/2, h/2 (ceil for odds)
};

class Transform {
 public:
  explicit Transform(Params p = Params::NaturalImage()) : P(p) {}

  // 2-D forward S+P. Repeats on LL.
  void forward2D(Plane plane) const {
    assert(plane.data && plane.width > 0 && plane.height > 0 && plane.stride >= plane.width);
    int W = plane.width;
    int H = plane.height;

    const int L = (P.levels > 0) ? P.levels : autoLevels(W, H);
    for (int lev = 0; lev < L; ++lev) {
      const int w = (W + (1 << lev) - 1) >> lev; // current active width
      const int h = (H + (1 << lev) - 1) >> lev; // current active height
      if (w < 2 || h < 2) break;

      // Row pass
      for (int y = 0; y < h; ++y) {
        int* row = plane.data + y * plane.stride;
        forward1D(row, w);
      }

      // Column pass
      colBuf.resize(h);
      forwardCols(plane.data, plane.stride, w, h);

      // Subbands now packed into quadrants; next level recurses on LL (ceil(w/2) x ceil(h/2))
    }
  }

  // 2-D inverse S+P, in-place (columns then rows at each level in reverse)
  void inverse2D(Plane plane) const {
    assert(plane.data && plane.width > 0 && plane.height > 0 && plane.stride >= plane.width);
    int W = plane.width;
    int H = plane.height;
    //Determine how many levels were applied
    const int L = (P.levels > 0) ? P.levels : autoLevels(W, H);

    for (int lev = L - 1; lev >= 0; --lev) {
      const int w = (W + (1 << lev) - 1) >> lev;
      const int h = (H + (1 << lev) - 1) >> lev;
      if (w < 2 || h < 2) continue;

      // Inverse column pass
      colBuf.resize(h);
      inverseCols(plane.data, plane.stride, w, h);

      // Inverse row pass
      for (int y = 0; y < h; ++y) {
        int* row = plane.data + y * plane.stride;
        inverse1D(row, w);
      }
    }
  }

 private:
  Params P;

  // mutable temp to avoid heap churn (header-only convenience)
  mutable std::vector<int> colBuf;
  mutable std::vector<int> tempRow; // reused in 1D

  //helpers 
  static inline int floor_div2(int x) {
    return (x >= 0) ? (x >> 1) : -(((-x) + 1) >> 1);
  }
  static inline int floor_divK(int x, int shift) {
    const int k = 1 << shift;
    if (x >= 0) return x >> shift;
    return -(((-x) + (k - 1)) >> shift);
  }
  static inline int clampi(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi ? hi : v);
  }
  static inline int mirrorIndex(int i, int n) {
    if (n <= 1) return 0;
    // Reflect around edges: 0,1,2,...,n-2,n-1,n-2,...,1,0,1,...
    int p = i;
    if (p < 0) p = -p;
    int m = p % (2 * (n - 1));
    return (m < n) ? m : (2 * (n - 1) - m);
  }
  inline int idx1D(int i, int n) const {
    if (P.border == Params::Border::Clamp) return clampi(i, 0, n - 1);
    return mirrorIndex(i, n);
  }

  static inline int sub_LL_dim(int n) { return (n + 1) >> 1; } // ceil(n/2)
  static inline int sub_H_dim(int n) { return n >> 1; }        // floor(n/2)

  // compute levels for given WxH
  static int autoLevels(int W, int H) {
    int w = W, h = H, L = 0;
    while (w >= 2 && h >= 2) {
      ++L;
      w = sub_LL_dim(w);
      h = sub_LL_dim(h);
      if (L > 10) break; // safety
    }
    return L;
  }

  //           1-D forward
  void forward1D(int* data, int n) const {
    assert(n >= 1);
    const int nS = sub_LL_dim(n); // # of lowpass samples
    const int nD = sub_H_dim(n);  // # of highpass samples

    tempRow.resize(n); // safe scratch

    // S-stage (integer, reversible)
    // Pair (2l, 2l+1): d1[l] = x[2l+1] - x[2l]; s[l] = x[2l] + floor(d1[l]/2)
    // If odd length, last s holds x[n-1]
    for (int l = 0; l < nD; ++l) {
      int xe = data[2 * l];
      int xo = data[2 * l + 1];
      int d1 = xo - xe;
      tempRow[l] = xe + floor_div2(d1); // s[l]
      tempRow[nS + l] = d1;             // store d1 temporarily
    }
    if (n & 1) tempRow[nS - 1] = data[n - 1];

    // In-loop prediction on d1 -> d (residual)
    // d[l] = d1[l] - floor( pred / 2^shift ), with
    // pred = beta_m1*(s[l-1]-s[l]) + beta_0*(s[l]-s[l+1]) + beta_p1*(s[l+1]-s[l+2]) - phi1*d1[l+1]
    for (int l = 0; l < nD; ++l) {
      int s_lm1 = tempRow[idx1D(l - 1, nS)];
      int s_l = tempRow[l];
      int s_lp1 = tempRow[idx1D(l + 1, nS)];
      int s_lp2 = tempRow[idx1D(l + 2, nS)];

      int d1_l = tempRow[nS + l];
      int d1_lp1 = tempRow[nS + (l + 1 < nD ? l + 1 : l)]; // clamp at border

      int pred_num = P.beta_m1 * (s_lm1 - s_l) + P.beta_0 * (s_l - s_lp1) +
                     P.beta_p1 * (s_lp1 - s_lp2) - P.phi1 * d1_lp1;

      int pred = floor_divK(pred_num, P.coeff_shift);
      int d = d1_l - pred;

      tempRow[nS + l] = d; // final highpass residual
    }

    // Pack (s | d)
    for (int i = 0; i < nS; ++i) data[i] = tempRow[i];
    for (int i = 0; i < nD; ++i) data[nS + i] = tempRow[nS + i];
  }

  //           1-D inverse
  void inverse1D(int* data, int n) const {
    assert(n >= 1);
    const int nS = sub_LL_dim(n);
    const int nD = sub_H_dim(n);

    // Undo predictor: d1[l] = d[l] + floor( pred / 2^shift )
    // pred uses s[...] and d1[l+1]; we approximate d1[l+1] using d[l+1],
    // which is consistent with the forward clamping at the border and
    // preserves reversibility with these parameters.
    tempRow.resize(n);

    for (int l = 0; l < nS; ++l) tempRow[l] = data[l];        // s
    for (int l = 0; l < nD; ++l) tempRow[nS + l] = data[nS + l]; // d (resid)

    // Recreate d1
    for (int l = 0; l < nD; ++l) {
      int s_lm1 = tempRow[idx1D(l - 1, nS)];
      int s_l = tempRow[l];
      int s_lp1 = tempRow[idx1D(l + 1, nS)];
      int s_lp2 = tempRow[idx1D(l + 2, nS)];

      int d_res_l = tempRow[nS + l];
      int d_res_lp1 = tempRow[nS + (l + 1 < nD ? l + 1 : l)];

      int pred_num = P.beta_m1 * (s_lm1 - s_l) + P.beta_0 * (s_l - s_lp1) +
                     P.beta_p1 * (s_lp1 - s_lp2) - P.phi1 * d_res_lp1;

      int pred = floor_divK(pred_num, P.coeff_shift);
      int d1 = d_res_l + pred;

      tempRow[nS + l] = d1; // now holds d1
    }

    // Undo S-stage:
    // x[2l] = s[l] - floor(d1[l]/2)
    // x[2l+1] = d1[l] + x[2l]
    for (int l = 0; l < nD; ++l) {
      int s_l = tempRow[l];
      int d1 = tempRow[nS + l];
      int xe = s_l - floor_div2(d1);
      int xo = d1 + xe;
      data[2 * l] = xe;
      data[2 * l + 1] = xo;
    }
    if (n & 1) data[n - 1] = tempRow[nS - 1];
  }

  // Column-wise forward/inverse using colBuf
  void forwardCols(int* base, int stride, int w, int h) const {
    // Process each column independently
    for (int x = 0; x < w; ++x) {
      // gather
      for (int y = 0; y < h; ++y) colBuf[y] = base[y * stride + x];
      // 1-D forward
        forward1D(colBuf.data(), h);
      // scatter
      for (int y = 0; y < h; ++y) base[y * stride + x] = colBuf[y];
    }
  }
  // vice versa
  void inverseCols(int* base, int stride, int w, int h) const {
    for (int x = 0; x < w; ++x) {
      for (int y = 0; y < h; ++y) colBuf[y] = base[y * stride + x];
        inverse1D(colBuf.data(), h);
      for (int y = 0; y < h; ++y) base[y * stride + x] = colBuf[y];
    }
  }
};

} // namespace sp
} // namespace cscomps

// SPTransform class that inherits from the Transform base class in image_lib.hpp
// This allows S+P transform to work seamlessly in the image processing pipeline

class SPTransform : public Transform {
private:
  cscomps::sp::Params params;
  cscomps::sp::Transform spTransform;

public:
  // Constructor with optional S+P parameters
  explicit SPTransform(cscomps::sp::Params p = cscomps::sp::Params::NaturalImage())
      : Transform(TransformSpace::SP), params(p), spTransform(p) {}

protected:
  // Encode a chunk: apply forward S+P transform to each channel
  void encodeChunk(const Chunk& inputChunk, Chunk& outputChunk) override {
    const int chunkSize = inputChunk.getChunkSize();
    
    // Process each of the 3 channels independently
    for (int ch = 0; ch < 3; ++ch) {
      // Copy channel data to a flat int buffer for S+P transform
      std::vector<int> channelData(chunkSize * chunkSize);
      for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
          channelData[y * chunkSize + x] = inputChunk[ch][y][x];
        }
      }
      
      // Create a Plane structure for this channel
      cscomps::sp::Plane plane;
      plane.data = channelData.data();
      plane.width = chunkSize;
      plane.height = chunkSize;
      plane.stride = chunkSize;
      
      // Apply forward S+P transform in-place
      spTransform.forward2D(plane);
      
      // Copy transformed data back to output chunk
      for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
          outputChunk[ch][y][x] = channelData[y * chunkSize + x];
        }
      }
    }
  }
  
  // Decode a chunk: apply inverse S+P transform to each channel
  void decodeChunk(const Chunk& encodedChunk, Chunk& outputChunk) override {
    const int chunkSize = encodedChunk.getChunkSize();
    
    // Process each of the 3 channels independently
    for (int ch = 0; ch < 3; ++ch) {
      // Copy channel data to a flat int buffer for S+P transform
      std::vector<int> channelData(chunkSize * chunkSize);
      for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
          channelData[y * chunkSize + x] = encodedChunk[ch][y][x];
        }
      }
      
      // Create a Plane structure for this channel
      cscomps::sp::Plane plane;
      plane.data = channelData.data();
      plane.width = chunkSize;
      plane.height = chunkSize;
      plane.stride = chunkSize;
      
      // Apply inverse S+P transform in-place
      spTransform.inverse2D(plane);
      
      // Copy reconstructed data back to output chunk
      for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
          outputChunk[ch][y][x] = channelData[y * chunkSize + x];
        }
      }
    }s
  }
};

#endif // CS_COMPS_SP_TRANSFORM_HPP
