# S+P Transform Testing Guide

## Current Status
- ✅ `sp_transform.hpp` has been reformatted with consistent indentation
- ✅ Created `test_simple.cpp` - a minimal test without external dependencies
- ❌ No C++ compiler currently installed

## Quick Setup: Install MinGW-w64 (Recommended)

### Option 1: Install via MSYS2 (Best for development)
1. Download MSYS2 from https://www.msys2.org/
2. Run the installer (installs to C:\msys64 by default)
3. Open "MSYS2 MSYS" from Start menu
4. Update packages:
   ```bash
   pacman -Syu
   # Close window when prompted, then reopen "MSYS2 MSYS"
   pacman -Syu
   ```
5. Install MinGW-w64 toolchain:
   ```bash
   pacman -S --needed base-devel mingw-w64-x86_64-toolchain
   ```
6. Add to your Git Bash PATH:
   ```bash
   echo 'export PATH="/c/msys64/mingw64/bin:$PATH"' >> ~/.bashrc
   source ~/.bashrc
   ```

### Option 2: TDM-GCC (Simpler)
1. Download TDM-GCC from https://jmeubank.github.io/tdm-gcc/
2. Install with default settings (adds to PATH automatically)
3. Restart your terminal

### Option 3: Visual Studio Build Tools
1. Download "Build Tools for Visual Studio 2022" from Microsoft
2. Install with "C++ build tools" workload
3. Use "x64 Native Tools Command Prompt" instead of Git Bash

## Testing Commands

### After installing MinGW-w64/TDM-GCC:
```bash
cd "C:/Users/lg/OneDrive/Carleton/CS/CS_Comps/Codes"

# Test basic compilation
g++ --version

# Compile the simple test (no dependencies)
g++ -std=c++17 -O2 -Wall -Wextra -I"S+P" "S+P/test_simple.cpp" -o "S+P/test_simple.exe"

# Run the test
./S+P/test_simple.exe
```

### Expected Output:
```
Testing S+P Transform...
Original data (first row): 0 1 2 3 4 5 6 7 
Running forward transform...
After forward (first row): [transformed coefficients]
Running inverse transform...
After inverse (first row): 0 1 2 3 4 5 6 7 
SUCCESS: SP round-trip test PASSED!
```

### With MSVC (if using Visual Studio Build Tools):
```cmd
cd C:\Users\lg\OneDrive\Carleton\CS\CS_Comps\Codes
cl /std:c++17 /O2 /W4 /I S+P S+P\test_simple.cpp
test_simple.exe
```

## Files Created:
- `S+P/test_simple.cpp` - Standalone test without external dependencies
- `S+P/test_sp_transform.cpp` - Original test with image_lib.hpp (requires stb_image.h)

## What the Test Does:
1. Creates an 8x8 integer array with a ramp pattern (0,1,2...77)
2. Runs S+P forward transform (converts to frequency domain)
3. Runs S+P inverse transform (reconstructs original data)
4. Verifies perfect reconstruction (all values match exactly)

## Troubleshooting:
- If you see "stb_image.h not found": Use `test_simple.cpp` instead of `test_sp_transform.cpp`
- If compiler not found after install: Restart terminal or check PATH
- If weird characters in output: Your terminal encoding might need adjustment

## Next Steps:
1. Install a compiler using one of the options above
2. Run the test commands
3. If test passes, you can uncomment the image_lib.hpp include and test with image data