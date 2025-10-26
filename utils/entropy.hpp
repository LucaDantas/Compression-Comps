#include <stdlib.h>
#include <stdio.h>
#include "dpcm.hpp"
#include <vector>
#include "rle.hpp"
#include "entropy.hpp"
#include "utils/image_lib.hpp"

struct EntropyEncoded {
	std::vector<std::vector<int *>> ACComponent; // ACComponent[channel][chunk][RLE_info - pairs] (EXCLUDES ALL DC INFO, i.e. chunk contains info for 63 pixels)
	std::vector<int *> DCComponent; // DCComponent[channel][DPCM_info]
};

void populateVector(int **arr, std::vector<int *>& targetVector, int size);
void populateChunk(int **arr, std::vector<int> *chunk)
EntropyEncoded *EntropyEncodeDCT(const ChunkedImage& chunkedImage);
int *EntropyEncode(const ChunkedImage& chunkedImage);
void EntropyDecodeDCT(ChunkedImage& chunkedImage, EntropyDecoded *encoded);
void EntropyDecode(ChunkedImage& chunkedImage, EntropyEncoded *encoded);

void populateVector(int **arr, std::vector<int *>& targetVector, int size) { // starts from j = 1 to ignore DC coefficient
	
		int j = 1;
		free(arr[0]);
		while (j < size*size && arr[j] != NULL) {
			targetVector.push_back(arr[j]);
			j++;
		}
	
}

void populateChunk(int **arr, std::vector<int> *chunk, int size) {
	
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


void EntropyDecodeDCT(ChunkedImage& chunkedImage, EntropyEncoded *encoded) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	
	int *coefficientsDC = NULL;
	int *coefficientsAC = NULL;
	int **coefficientsACByChunk = (int **)malloc(sizeof(int *)*size*size);
	int first[2] = {16, 16};
	coefficientsACByChunk[0] = first;
	int **tempArray = NULL;
	int j = 0;
	int k = 0;
	int curRleSum = 0;

	std::vector<int *> DCComponent = (*encoded).DCComponent;
	std::vector<std::vector<int *>> ACComponent = (*encoded).ACComponent;
	
	std::vector<int> *curChunk = NULL;
	
	for (int channel = 0; channel < 3; channel++) {

		coefficientsDC = dpcm::decoder(DCComponent[channel], size, predictionSize);
		k = 0;
		
		for (int i = 0; i < numChunks; i++) {
			chunkedImage.getChunkAt(i)[channel][0][0] = coefficientsDC[i];
		}
		
		free(coefficientsDC);
				
		for (int i = 0; i < numChunks; i++) {
			curChunk = chunkedImage.getChunkAt(i)[channel].data();
			
			j = 0;
			curRleSum = 0;
			
			while (curRleSum < size*size-1 && j < size*size-1) {
				coefficientsACByChunk[j+1] = ACComponent[channel][k];
				curRleSum += ACComponent[channel][k][0] + 1;
				k++;
				j++;
			}
			
			coefficientsAC = rle::decoder(coefficientsACByChunk, size);
			tempArray = unflattenArray(coefficientsAC, size);
			
			populateChunk(tempArray, curChunk, size);

			
			for (int i = 0; i < size; i++) {
				free(tempArray[i]);
			}

			free(tempArray);
			free(coefficientsAC);
		}
	}
}

void EntropyDecode(ChunkedImage& chunkedImage, EntropyEncoded *encoded) {
	
	if (chunkedImage.transformSpace == DCT) {
		EntropyDecodeDCT(ChunkedImage chunkedImage, EntropyDecoded *encoded);
	} 
}