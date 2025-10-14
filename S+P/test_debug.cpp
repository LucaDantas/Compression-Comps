#include <cassert>
#include <cstdio>
#include <vector>

#include "sp_transform.hpp"

using namespace cscomps::sp;

void test_1d_only() {
  printf("Testing 1D transform only...\n");
  
  // Create a simple 1D test case
  std::vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7};
  std::vector<int> original = data;
  
  printf("Original: ");
  for (int x : data) printf("%d ", x);
  printf("\n");
  
  Transform t;
  
  // Test 1D forward and inverse directly
  // We'll need to access the private methods, so let's make them public temporarily
  // For now, let's test with a 1x8 "image"
  
  Plane p;
  p.data = data.data();
  p.width = 8;
  p.height = 1;
  p.stride = 8;
  
  printf("Testing 1x8 image (effectively 1D)...\n");
  t.forward2D(p);
  
  printf("After forward: ");
  for (int x : data) printf("%d ", x);
  printf("\n");
  
  t.inverse2D(p);
  
  printf("After inverse: ");
  for (int x : data) printf("%d ", x);
  printf("\n");
  
  bool success = true;
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] != original[i]) {
      printf("Mismatch at %zu: expected %d, got %d\n", i, original[i], data[i]);
      success = false;
    }
  }
  
  if (success) {
    printf("✅ 1D test PASSED!\n");
  } else {
    printf("❌ 1D test FAILED!\n");
  }
}

void test_minimal_2d() {
  printf("\nTesting minimal 2x2 transform...\n");
  
  std::vector<int> data = {0, 1, 2, 3}; // 2x2 image
  std::vector<int> original = data;
  
  printf("Original 2x2: [%d %d; %d %d]\n", data[0], data[1], data[2], data[3]);
  
  Plane p;
  p.data = data.data();
  p.width = 2;
  p.height = 2;
  p.stride = 2;
  
  Transform t;
  t.forward2D(p);
  
  printf("After forward: [%d %d; %d %d]\n", data[0], data[1], data[2], data[3]);
  
  t.inverse2D(p);
  
  printf("After inverse: [%d %d; %d %d]\n", data[0], data[1], data[2], data[3]);
  
  bool success = true;
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] != original[i]) {
      printf("Mismatch at %zu: expected %d, got %d\n", i, original[i], data[i]);
      success = false;
    }
  }
  
  if (success) {
    printf("✅ 2x2 test PASSED!\n");
  } else {
    printf("❌ 2x2 test FAILED!\n");
  }
}

int main() {
  test_1d_only();
  test_minimal_2d();
  return 0;
}