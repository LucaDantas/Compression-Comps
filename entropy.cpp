#include <stdlib.h>
#include <stdio.h>
#include "dpcm.hpp"
// #include "rle.hpp"
#include "utils/image_lib.hpp"

int *EntropyEncodeDCT(const ChunkedImage& chunkedImage) {
	int numChunks = chunkedImage.getTotalChunks();
	
	int *coefficientsDC = (int *)malloc(sizeof(int)*numChunks);
	
	for (int channel = 0; channel < 3; channel++) {
		for (int i = 0; i < numChunks; i++) {
			
			coefficientsDC[i] = chunkedImage.getChunkAt(i)[channel][0][0];
			int predictionSize = 4;
			int *predictedDC = dpcm::encoder(coefficientsDC, numChunks, predictionSize);
			
		}
	}
}


int *EntropyEncode(const ChunkedImage& chunkedImage) {
	
	if (chunkedImage.transformSpace == DCT) {
		EntropyEncodeDCT(const ChunkedImage& chunkedImage);
	} 
	
	return NULL;
	
}

int main(int argc, char **argv) {
	
    if (argc > 1)
    {
        printf("Usage message\n");
        return 1;
    }
	
	// test_flatten();
	// test_linear();
	test_encoder();
	
	return 0;
	
}