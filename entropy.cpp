#include <stdlib.h>
#include <stdio.h>
#include "dpcm.hpp"
#include <vector>
#include "rle.hpp"
#include "entropy.hpp"
#include "utils/image_lib.hpp"

void populateVector(int **arr, std::vector<int *>& targetVector, int size) { // starts from j = 1 to ignore DC coefficient
	
		int j = 1;
		free(arr[0]);
		while (j < size*size && arr[j] != NULL) {
			targetVector.push_back(arr[j]);
			j++;
		}
	
}

void populateChunk(int **arr, std::vector<int> *chunk) {
	
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (i == 0 && j == 0) {
				continue;
			} else {
				chunk[i][j] = arr[i][j];
			}
		}
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
			curChunk = chunkedImage.getChunkAt(i)[channel].data();
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


void EntropyDecodeDCT(ChunkedImage chunkedImage, EntropyDecoded *encoded) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	
	int *coefficientsDC = NULL;
	int *coefficientsAC = NULL;
	int **coefficientsACByChunk = (int **)malloc(sizeof(int *)*size*size);
	coefficientsACByChunk[0] = [16, 16];
	
	int **tempArray = NULL;

	std::vector<int *> DCComponent = (*encoded).DCComponent;
	std::vector<std::vector<int *>> ACComponent = (*encoded).ACComponent;
	
	std::vector<int> *curChunk = NULL;
	
	for (int channel = 0; channel < 3; channel++) {

		coefficientsDC = dpcm::decoder(DCComponent[channel], size, predictionSize);
		
		for (int i = 0; i < numChunks; i++) {
			chunkedImage.getChunkAt(i)[channel][0][0] = coefficientsDC[i];
		}
		
		free(coefficientsDC);
		
		for (int i = 0; i < numChunks; i++) {
			curChunk = chunkedImage.getChunkAt(i)[channel].data();
			
			for (int j = 0; j < size*size-1; j++) {
				coefficientsACByChunk[j+1] = ACComponent[channel][i*63 + j];
			}
			
			coefficientsAC = rle::decoder(coefficientsACByChunk, size);
			tempArray = unflattenArray(coefficientsAC, size);
			
			populateChunk(tempArray, curChunk);
			
			for (int i = 0; i < size; i++) {
				free(tempArray[i]);
			}
			free(tempArray);
			free(coefficientsAC);
		}
	}
}

void EntropyDecode(ChunkedImage chunkedImage, EntropyEncoded *encoded) {
	
	if (chunkedImage.transformSpace == DCT) {
		EntropyDecodeDCT(ChunkedImage chunkedImage, EntropyDecoded *encoded);
	} 
}