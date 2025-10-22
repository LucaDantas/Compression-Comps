#include <vector>
void printArray(int *arr, int size);
int sum(int *arr, int size);
int dotProduct(int *arr1, int *arr2, int size);
int *createLinearArray(int size);
int *zigzagFlattenArray(int **arr, int size);
int *zigzagFlattenArray(const std::vector<int> *arr, int size);
int **unflattenArray(int *arr, int size);
int linearPredictor(int *arr, int size); //size means the number of previous variables to use
namespace dpcm {
	int* encoder(int *arr, int size, int prediction_size);
	int* decoder(int *arr, int size, int prediction_size);
}