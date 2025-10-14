#include <cassert>
#include <cstdio>
#include <vector>

#include "sp_transform.hpp"

using namespace cscomps::sp;

int main() {
  printf("Testing S+P Transform...\n");
  
  // Create a small 8x8 test image with stride == width
  const int W = 8, H = 8, S = W;
  std::vector<int> buf(W * H);
  std::vector<int> original(W * H);

  // Fill with a simple ramp pattern
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int val = (y * 10 + x);
      buf[y * S + x] = val;
      original[y * S + x] = val;
    }
  }

  printf("Original data (first row): ");
  for (int x = 0; x < W; ++x) {
    printf("%d ", original[x]);
  }
  printf("\n");

  Plane p;
  p.data = buf.data();
  p.width = W;
  p.height = H;
  p.stride = S;

  Transform t; // default params

  // Forward transform
  printf("Running forward transform...\n");
  t.forward2D(p);
  
  printf("After forward (first row): ");
  for (int x = 0; x < W; ++x) {
    printf("%d ", buf[x]);
  }
  printf("\n");

  // Inverse transform
  printf("Running inverse transform...\n");
  t.inverse2D(p);
  
  printf("After inverse (first row): ");
  for (int x = 0; x < W; ++x) {
    printf("%d ", buf[x]);
  }
  printf("\n");

  // Verify round-trip
  int errors = 0;
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int expected = original[y * S + x];
      int got = buf[y * S + x];
      if (expected != got) {
        printf("ERROR: Mismatch at (%d,%d): expected %d, got %d\n", y, x, expected, got);
        errors++;
      }
    }
  }

  if (errors == 0) {
    printf("SUCCESS: SP round-trip test PASSED!\n");
    return 0;
  } else {
    printf("FAILED: %d mismatches found\n", errors);
    return 1;
  }
}