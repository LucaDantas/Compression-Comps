#include <cassert>
#include <cstdio>
#include <vector>

#include "sp_transform.hpp"
#include "image_lib.hpp"

using namespace cscomps::sp;

int main() {
  // Create a small 8x8 test image with stride == width
  const int W = 8, H = 8, S = W;
  std::vector<int> buf(W * H);

  // Fill with a simple ramp pattern
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      buf[y * S + x] = (y * 10 + x);
    }
  }

  Plane p;
  p.data = buf.data();
  p.width = W;
  p.height = H;
  p.stride = S;

  Transform t; // default params

  // Forward then inverse
  t.forward2D(p);
  t.inverse2D(p);

  // Verify round-trip
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int expected = (y * 10 + x);
      int got = buf[y * S + x];
      if (expected != got) {
        std::fprintf(stderr, "Mismatch at (%d,%d): expected %d, got %d\n", y, x, expected, got);
        return 1;
      }
    }
  }

  std::puts("SP round-trip PASS");

  // Adapter path: use TransformedChunk from image_lib.hpp
  TransformedChunk chunk(8, TransformSpace::rawRGB);
  // Fill channel 0 with a pattern
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      chunk.getChannel(0)[y][x] = static_cast<uint8_t>((y * 7 + x * 3) & 0xFF);
    }
  }

  // Forward on channel 0, then inverse back
  std::vector<int> coeffs; int stride2, w2, h2;
  t.forward2D_fromChunk(chunk, 0, coeffs, stride2, w2, h2);

  TransformedChunk outChunk(8, TransformSpace::rawRGB);
  t.inverse2D_toChunk(coeffs, stride2, w2, h2, outChunk, 0);

  // Verify equality
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      uint8_t a = chunk.getChannel(0)[y][x];
      uint8_t b = outChunk.getChannel(0)[y][x];
      if (a != b) {
        std::fprintf(stderr, "Chunk adapter mismatch at (%d,%d): %u vs %u\n", y, x, a, b);
        return 2;
      }
    }
  }
  std::puts("SP adapter round-trip PASS");
  return 0;
}
