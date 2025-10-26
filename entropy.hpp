#include <vector>

struct EntropyEncoded {
	std::vector<std::vector<int *>> ACComponent; // ACComponent[channel][pair #][RLE_info (preceding zeroes, value)] (EXCLUDES ALL DC INFO, i.e. chunk contains info for 63 pixels)
	std::vector<int *> DCComponent; // DCComponent[channel][DPCM_info]
};

void populateVector(int **arr, std::vector<int *>& targetVector, int size);
void populateChunk(int **arr, std::vector<int> *chunk)
EntropyEncoded *EntropyEncodeDCT(const ChunkedImage& chunkedImage);
int *EntropyEncode(const ChunkedImage& chunkedImage);
void EntropyDecodeDCT(ChunkedImage& chunkedImage, EntropyDecoded *encoded);
void EntropyDecode(ChunkedImage& chunkedImage, EntropyEncoded *encoded);
