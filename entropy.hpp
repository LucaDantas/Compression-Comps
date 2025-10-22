#include <vector>

struct EntropyEncoded {
	std::vector<std::vector<int *>> ACComponent; // ACComponent[channel][chunk][RLE_info] (EXCLUDES ALL DC INFO, i.e. chunk contains info for 63 pixels)
	std::vector<int *> DCComponent; // DCComponent[channel][DPCM_info]
};

void populateVector(int **arr, std::vector<int *>& targetVector, int size);
EntropyEncoded *EntropyEncodeDCT(const ChunkedImage& chunkedImage);
int *EntropyEncode(const ChunkedImage& chunkedImage);