#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#define _USE_MATH_DEFINES

std::vector<std::vector<int> >* dct(std::vector<std::vector<int> >* f) {
    double n = (double)f->size();
    std::vector<std::vector<int> >* F = new std::vector<std::vector<int> >(n, std::vector<int>(n, 0));
    double sum, cu, cv;
    int pixel;
    for (int u=0; u<n; u++) {
        for (int v=0; v<n; v++) {
            sum = 0;
            if (u == 0) cu = 1 / std::sqrt(2); else cu = 1;
            if (v == 0) cv = 1 / std::sqrt(2); else cv = 1;
            for (int x=0; x<n; x++) {
                for (int y=0; y<n; y++) {
                    pixel = (*f)[x][y] - 128;
                    sum += pixel * std::cos((M_PI * (2*x + 1) * u) / (2*n)) * std::cos((M_PI * (2*y + 1) * v) / (2*n));
                }
            }
            // std:: cout << "Sum for (" << i << ", " << j << "): " << sum << '\n';
            // std:: cout << "Final value before casting: " << ((2 / n) * ci * cj * sum) << '\n';
            // std:: cout << (2 / n) << ' ' << ci << ' ' << cj << "\n\n";
            (*F)[u][v] = std::round((2 / n) * cu * cv * sum);
        }
    }

    return F;
}

std::vector<std::vector<int> >* idct(std::vector<std::vector<int> >* F) {
    double n = (double)F->size();
    std::vector<std::vector<int> >* f = new std::vector<std::vector<int> >(n, std::vector<int>(n, 0));
    double sum, cu, cv;
    int pixel;
    for (int x=0; x<n; x++) {
        for (int y=0; y<n; y++) {
            sum = 0;
            for (int u=0; u<n; u++) {
                for (int v=0; v<n; v++) {
                    if (u == 0) cu = 1 / std::sqrt(2); else cu = 1;
                    if (v == 0) cv = 1 / std::sqrt(2); else cv = 1;
                    pixel = (*F)[u][v];
                    sum += pixel * cu * cv * std::cos((M_PI * (2*x + 1) * u) / (2*n)) * std::cos((M_PI * (2*y + 1) * v) / (2*n));
                }
            }
            // std:: cout << "Sum for (" << i << ", " << j << "): " << sum << '\n';
            // std:: cout << "Final value before casting: " << ((2 / n) * ci * cj * sum) << '\n';
            // std:: cout << (2 / n) << ' ' << ci << ' ' << cj << "\n\n";
            (*f)[x][y] = std::round((2 / n) * sum) + 128;
        }
    }

    return f;
}

void displayMatrix(std::vector<std::vector<int> >* mat) {
    int n = mat->size();
    for (int k=0; k<n; k++) {
        std::cout << "[ ";
        for (int m=0; m<8; m++) {
            std::cout << (*mat)[k][m] << ' ';
        }
        std::cout << "]\n";
    }
}

int main() {
    std::cout << "Constant Pixels:\n";
    std::vector<std::vector<int> > mat(8, std::vector<int>(8, 255));
    displayMatrix(&mat);

    std::cout << "\nResult:\n";
    std::vector<std::vector<int> >* res = dct(&mat);
    displayMatrix(res);

    std::cout << "\nInverse:\n";
    std::vector<std::vector<int> >* original = idct(res);
    displayMatrix(original);

    std::cout << "\nChanging Pixels:\n";
    std::vector<std::vector<int> > mat2(8, std::vector<int>(8, 0));
    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++) {
            mat2[i][j] += i + j;
        }
    }
    displayMatrix(&mat2);

    std::cout << "\nResult:\n";
    std::vector<std::vector<int> >* res2 = dct(&mat2);
    displayMatrix(res2);

    std::cout << "\nInverse:\n";
    std::vector<std::vector<int> >* original2 = idct(res2);
    displayMatrix(original2);

    delete res;
    delete original;
    delete res2;
    delete original2;

    return 0;
}