#include <stdlib.h>
#include <stdio.h>
#include "dpcm.hpp"
#include <vector>
#include "rle.hpp"
#include "entropy.hpp"
#include "utils/image_lib.hpp"

void populateVector(int **arr, std::vector<int *>& targetVector, int size) {
	
		int j = 1;
		free(arr[0]);
		while (j < size*size && arr[j] != NULL) {
			targetVector.push_back(arr[j]);
			j++;
		}
	
}

EntropyEncoded *EntropyEncodeDCT(const ChunkedImage& chunkedImage) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	std::vector<int *> predictedDC(3, NULL);
	
	int *coefficientsDC = NULL;
	int *flat = NULL;
	int **rleResult = NULL;
	const std::vector<int> *curChunk = NULL;
	std::vector<std::vector<int *>> finalVector(3, std::vector<int *>());

	for (int channel = 0; channel < 3; channel++) {
		coefficientsDC = (int *)malloc(sizeof(int)*numChunks);
		for (int i = 0; i < numChunks; i++) {
			coefficientsDC[i] = chunkedImage.getChunkAt(i)[channel][0][0];
		}
		predictedDC[channel] = dpcm::encoder(coefficientsDC, size, predictionSize);
		free(coefficientsDC);
		for (int i = 0; i < numChunks; i++) {
			curChunk = [channel].data();
			flat = zigzagFlattenArray(curChunk, size);
			
			rleResult = rle::encoder(flat, size);
			populateVector(rleResult, finalVector[channel], size);
			
			free(flat);
		}
		
	}
	
	EntropyEncoded *result = (EntropyEncoded *)malloc(sizeof(EntropyEncoded));
	(*result).ACComponent = finalVector;
	(*result).DCComponent = predictedDC;
	
	return result;
	
}


int *EntropyEncode(const ChunkedImage& chunkedImage) {
	
	EntropyEncoded *result = NULL;
	
	if (chunkedImage.transformSpace == DCT) {
		result = EntropyEncodeDCT(const ChunkedImage& chunkedImage);
	} 
	
	return result;
	
}


ChunkedImage EntropyDecode()