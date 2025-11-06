#ifndef CS_COMPS_SP_TRANSFORM_HPP
#define CS_COMPS_SP_TRANSFORM_HPP

// All data are ints; no external libraries.
// Public API: forward2D / inverse2D operating in-place on one image plane.
// Pipeline contract: chunked Image (Y / Cb / Cr)
// by Jiale Wan Oct 2025
// SP Transform for 2-D images

#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>
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

class SPCore {
 public:
  explicit SPCore(Params p = Params::NaturalImage()) : P(p) {}

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
  
// To map 1D index with border handling to [0, n-1]
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

  // 1-D forward
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

  //1-D inverse
  void inverse1D(int* data, int n) const {
    assert(n >= 1);
    const int nS = sub_LL_dim(n);
    const int nD = sub_H_dim(n);

    // Undo predictor: d1[l] = d[l] + floor( pred / 2^shift )
    // pred uses s[...] and d1[l+1]; we approximate d1[l+1] using d[l+1],
    // which is consistent with the forward clamping at the border and
    // preserves reversibility with these parameters.
    tempRow.resize(n);

    for (int l = 0; l < nS; ++l) tempRow[l] = data[l]; // s
    for (int l = 0; l < nD; ++l) tempRow[nS + l] = data[nS + l]; // d (resid)

    // Recreate d1
    for (int l = nD - 1; l >= 0; --l) {
      int s_lm1 = tempRow[idx1D(l - 1, nS)];
      int s_l = tempRow[l];
      int s_lp1 = tempRow[idx1D(l + 1, nS)];
      int s_lp2 = tempRow[idx1D(l + 2, nS)];
      // inverse prediction
      const int d_res_l = tempRow[nS + l];
      int s_part = P.beta_m1 * (s_lm1 - s_l) + P.beta_0 * (s_l - s_lp1) +
                   P.beta_p1 * (s_lp1 - s_lp2);

      int d1_l;
      // edge
      if (l == nD - 1) {
        // Forward used clamp d1[l+1]=d1[l]; implicit equation: d = d1 - floor((s_part - phi1*d1)/K)
        // The closed-form used previously is incorrect for many integer combinations.
        // Solve by fixed-point iteration on pred = floor((s_part - phi1 * d1)/K).
        // Start with an initial pred estimate using d_res_l, then iterate until
        // pred stabilizes (typically converges quickly).
        int pred = floor_divK(s_part - P.phi1 * d_res_l, P.coeff_shift);
        d1_l = d_res_l + pred;
        for (int it = 0; it < 10; ++it) {
          int pred2 = floor_divK(s_part - P.phi1 * d1_l, P.coeff_shift);
          if (pred2 == pred) break;
          pred = pred2;
          d1_l = d_res_l + pred;
        }
      } else {
        int d1_lp1 = tempRow[nS + l + 1];
        int pred_num = s_part - P.phi1 * d1_lp1;
        int pred = floor_divK(pred_num, P.coeff_shift);
        d1_l = d_res_l + pred;
      }

      tempRow[nS + l] = d1_l; // now holds d1
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
  cscomps::sp::SPCore spTransform;

public:
  // Quantization parameterization
  struct QuantParams {
    int q_LL = 1;   // base LL step
    int q_HL = 4;   // base HL step
    int q_LH = 4;   // base LH step
    int q_HH = 6;   // base HH step
    int deadzone = 1; // dead-zone multiplier
    float scale = 1.0f; // global scale
    float level_gamma = 1.0f; // geometric per-level multiplier
    QuantParams() = default;
  };

  struct QTable {
    int ll;
    int hl;
    int lh;
    int hh;
    int dz;
  };

  // Construct with optional S+P params and optional quant params
  explicit SPTransform(cscomps::sp::Params p = cscomps::sp::Params::NaturalImage())
      : Transform(TransformSpace::SP), params(p), spTransform(p), qparams() {}
  explicit SPTransform(const QuantParams& qp, cscomps::sp::Params p = cscomps::sp::Params::NaturalImage())
      : Transform(TransformSpace::SP), params(p), spTransform(p), qparams(qp) {}

  // Convenience factory for QuantParams
  static QuantParams MakeQuantParams(float scale) {
    QuantParams qp;
    qp.scale = scale;
    return qp;
  }

  void setQuantParams(const QuantParams& qp) { qparams = qp; }
  QuantParams getQuantParams() const { return qparams; }

  // Build a QTable for a given recursion level
  QTable MakeQTableForLevel(int level) const {
    QTable qt;
    float level_factor = 1.0f;
    // Only use geometric scaling when level_gamma > 0.0
    if (qparams.level_gamma > 0.0f) {
      level_factor = std::pow(qparams.level_gamma, static_cast<float>(level));
    } else {
      // level_gamma <= 0.0 signals "no per-level scaling"
      level_factor = 1.0f;
    }
    auto scaled = [&](int base) -> int {
      float raw = static_cast<float>(base) * qparams.scale * level_factor;
      int v = static_cast<int>(std::round(raw));
      return (v <= 0) ? 0 : v;
    };
    qt.ll = scaled(qparams.q_LL);
    qt.hl = scaled(qparams.q_HL);
    qt.lh = scaled(qparams.q_LH);
    qt.hh = scaled(qparams.q_HH);
    qt.dz = (qparams.deadzone <= 0) ? 0 : qparams.deadzone;
    return qt;
  }

private:
  QuantParams qparams;
public:

public:
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
    }
  }

  // helpers
  static inline int q_ll_plain(int x, int step) {
    return (step > 0) ? static_cast<int>(std::round(static_cast<float>(x) / static_cast<float>(step))) : x;
  }
  static inline int dq_ll_plain(int q, int step) {
    if (step <= 0) return q;
    // reconstruct to bin center (midpoint)
    return q * step + (step >> 1);
  }

  static inline int q_hp_deadzone(int x, int step, int dz_mult) {
    if (step <= 0) return x;
    const int z = dz_mult * step;  // half-width of zero bin
    const int ax = (x >= 0) ? x : -x;
    if (ax < z) return 0;
    int q = static_cast<int>(std::floor(static_cast<float>(ax - z) / static_cast<float>(step))) + 1;
    return (x < 0) ? -q : q;
  }
  static inline int dq_hp_midpoint(int q, int step) {
    if (step <= 0) return q;
    if (q == 0)    return 0;
    int aq = (q >= 0) ? q : -q;
    int sign = (q >= 0) ? 1 : -1;
    // For q>0, midpoint = sign * (dz*step + (q-1)*step + 0.5*step)
    // deadzone is handled in quantization step, not here
    int midpoint = sign * ((aq - 1) * step + (step / 2));
    return midpoint;
  }

  // Try to make quantization parametized
  static inline int QuantizeLL(float v, int step) {
    if (step <= 0) return static_cast<int>(std::round(v));
    return static_cast<int>(std::round(v / static_cast<float>(step)));
  }

  static inline int QuantizeDeadZone(float v, int step, int dz) {
    if (step <= 0) return static_cast<int>(std::round(v));
    const float t = static_cast<float>(dz) * static_cast<float>(step);
    if (std::abs(v) < t) return 0;
    float s = (std::abs(v) - t) / static_cast<float>(step) + 1.0f;
    int q = static_cast<int>(std::floor(s));
    return (v >= 0.0f) ? q : -q;
  }

  static inline float DequantizeLL(int q, int step) {
    if (step <= 0) return static_cast<float>(q);
    return static_cast<float>(q * step);
  }

  static inline float DequantizeDeadZone(int q, int step, int dz) {
    if (step <= 0) return static_cast<float>(q);
    if (q == 0) return 0.0f;
    int aq = (q >= 0) ? q : -q;
    float sign = (q >= 0) ? 1.0f : -1.0f;
    float edge = static_cast<float>(dz) * static_cast<float>(step);
    // place the reconstruction value at edge + (aq-1 + 0.5)*step
    float val = edge + (static_cast<float>(aq - 1) + 0.5f) * static_cast<float>(step);
    return sign * val;
  }

  // Recursive band iterator over LL 
  template <typename QF_LL, typename QF_HP>
  static void forEachBandLevel(
      int* base, int stride, int W, int H, int levels,
      QF_LL q_ll, QF_HP q_hp) {

    int w = W, h = H;
    for (int lev = 0; lev < levels; ++lev) {
      const int wLL = (w + 1) >> 1;
      const int hLL = (h + 1) >> 1;

      for (int y = 0; y < h; ++y) {
        int* row = base + y * stride;
        for (int x = 0; x < w; ++x) {
          const bool isLL = (x < wLL) && (y < hLL);
          const bool isHL = (x >= wLL) && (y < hLL);
          const bool isLH = (x <  wLL) && (y >= hLL);
          const bool isHH = (x >= wLL) && (y >= hLL);

          row[x] = isLL
            ? q_ll(row[x], lev)
            : q_hp(row[x], lev, isHL, isLH, isHH);
        }
      }
      w = wLL; h = hLL; // recurse to next LL
    }
  }

  // Public API on SPTransform
  private:
    int levelsFor(int W, int H) const {
      if (params.levels > 0) return params.levels;
      int w = W, h = H, L = 0;
      while (w >= 2 && h >= 2 && L < 10) {
        ++L; w = (w + 1) >> 1; h = (h + 1) >> 1;
      }
      return L;
    }
  public:
    //override optional: set quant params directly
    void setQuantTable(const QTable& /*unused*/) { /* kept for API compatibility */ }

    // Quantize a single chunk (override of Transform::quantizeChunk)
    void quantizeChunk(const Chunk& inputChunk, Chunk& outputChunk, double scale) override {
      const int chunkSize = inputChunk.getChunkSize();
      const int W = chunkSize;
      const int H = chunkSize;
      const int S = chunkSize;
      const int L = levelsFor(W, H);

      std::vector<QTable> qtables;
      qtables.reserve(L);
      for (int lev = 0; lev < L; ++lev) {
        qtables.push_back(MakeQTableForLevel(lev));
      }

      for (int ch = 0; ch < 3; ++ch) {
        std::vector<int> plane(chunkSize * chunkSize);
        for (int y = 0; y < chunkSize; ++y)
          for (int x = 0; x < chunkSize; ++x)
            plane[y * chunkSize + x] = inputChunk[ch][y][x];

        auto q_ll = [&](int v, int lev) -> int {
          const QTable& qt = qtables[lev];
          return QuantizeLL(static_cast<float>(v), qt.ll);
        };

        auto q_hp = [&](int v, int lev, bool isHL, bool isLH, bool isHH) -> int {
          const QTable& qt = qtables[lev];
          int step = isHL ? qt.hl : (isLH ? qt.lh : qt.hh);
          return QuantizeDeadZone(static_cast<float>(v), step, qt.dz);
        };

        forEachBandLevel(plane.data(), S, W, H, L, q_ll, q_hp);

        // write back (store ints)
        for (int y = 0; y < chunkSize; ++y)
          for (int x = 0; x < chunkSize; ++x)
            outputChunk[ch][y][x] = plane[y * chunkSize + x] / scale;
      }
    }

    // Dequantize a single chunk (override of Transform::dequantizeChunk)
    void dequantizeChunk(const Chunk& encodedChunk, Chunk& outputChunk, double scale) override {
      const int chunkSize = encodedChunk.getChunkSize();
      const int W = chunkSize;
      const int H = chunkSize;
      const int S = chunkSize;
      const int L = levelsFor(W, H);

      std::vector<QTable> qtables;
      qtables.reserve(L);
      for (int lev = 0; lev < L; ++lev) {
        qtables.push_back(MakeQTableForLevel(lev));
      }

      for (int ch = 0; ch < 3; ++ch) {
        std::vector<int> plane(chunkSize * chunkSize);
        for (int y = 0; y < chunkSize; ++y)
          for (int x = 0; x < chunkSize; ++x)
            plane[y * chunkSize + x] = encodedChunk[ch][y][x];

        auto dq_ll = [&](int q, int lev) -> int {
          const QTable& qt = qtables[lev];
          float v = DequantizeLL(q, qt.ll);
          return static_cast<int>(std::round(v));
        };

        auto dq_hp = [&](int q, int lev, bool isHL, bool isLH, bool isHH) -> int {
          const QTable& qt = qtables[lev];
          int step = isHL ? qt.hl : (isLH ? qt.lh : qt.hh);
          float v = DequantizeDeadZone(q, step, qt.dz);
          return static_cast<int>(std::round(v));
        };

        forEachBandLevel(plane.data(), S, W, H, L, dq_ll, dq_hp);

        for (int y = 0; y < chunkSize; ++y)
          for (int x = 0; x < chunkSize; ++x)
            outputChunk[ch][y][x] = plane[y * chunkSize + x] * scale;
      }
    }

    // Estimate bits-per-pixel from a quantized ChunkedImage using symbol entropy (bits/sample * channels)
    static double EstimateBpp(const ChunkedImage& qimg) {
      // Leverage Image entropy implementation (counts over all channel values)
      Image tmp(qimg);
      double bits_per_sample = tmp.getEntropy(); // bits per sample (channel)
      return bits_per_sample * 3.0; // bits per pixel (3 channels)
    }

};

#endif // CS_COMPS_SP_TRANSFORM_HPP
