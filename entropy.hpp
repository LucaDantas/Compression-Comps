#include <vector>

struct EntropyEncoded {
	std::vector<std::vector<int *>> ACComponent;
	std::vector<int *> DCComponent;
};

void populateVector(int **arr, std::vector<int *>& targetVector, int size);
EntropyEncoded *EntropyEncodeDCT(const ChunkedImage& chunkedImage);
int *EntropyEncode(const ChunkedImage& chunkedImage);