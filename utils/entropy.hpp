#ifndef ENTROPY_HPP
#define ENTROPY_HPP

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "dpcm.hpp"
#include <vector>
#include "rle.hpp"
#include "image_lib.hpp"

struct EntropyEncoded {
	std::vector<std::vector<std::pair<int, int>>> ACComponent; // ACComponent[channel][pair #][RLE_info (preceding zeroes, value)] (EXCLUDES ALL DC INFO, i.e. chunk contains info for 63 pixels)
	std::vector<int *> DCComponent; // DCComponent[channel][DPCM_info]
};

EntropyEncoded EntropyEncodeDCT(const ChunkedImage& chunkedImage);
EntropyEncoded EntropyEncode(const ChunkedImage& chunkedImage);
void EntropyDecodeDCT(ChunkedImage& chunkedImage, const EntropyEncoded& encoded);
void EntropyDecode(ChunkedImage& chunkedImage, const EntropyEncoded& encoded);

void populateVector(const std::vector<std::pair<int, int>>& arr, std::vector<std::pair<int, int>>& targetVector, int size) {
	
		int j = 0;
		while (j < size*size-1 && arr[j].first > -1) {
			targetVector.push_back(arr[j]);
			j++;
		}
	
}

void populateChunk(const std::vector<std::vector<int>>& arr, std::vector<std::vector<int>>& chunk, int size) {
	
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

EntropyEncoded EntropyEncodeDCT(const ChunkedImage& chunkedImage) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	std::vector<int *> predictedDC(3, NULL);
	
	int *coefficientsDC = NULL;
	int *flat = NULL;
	std::vector<std::pair<int, int>> rleResult;
	const std::vector<int> *curChunk = NULL;
	std::vector<std::vector<std::pair<int, int>>> finalVector(3, std::vector<std::pair<int, int>>());
	int curRleSum = 0;
	
	for (int channel = 0; channel < 3; channel++) {
		coefficientsDC = (int *)malloc(sizeof(int)*numChunks);
		for (int i = 0; i < numChunks; i++) {
			coefficientsDC[i] = chunkedImage.getChunkAt(i)[channel][0][0];
		}
		predictedDC[channel] = dpcm::encoder(coefficientsDC, numChunks, predictionSize);
		free(coefficientsDC);
		for (int i = 0; i < numChunks; i++) {
			curChunk = chunkedImage.getChunkAt(i)[channel].data();
			flat = zigzagFlattenArray(curChunk, size);
			
			if (channel == 0 && i+1 == 2) {
				for (int u = 0; u < size; u++) {
					printArray(curChunk[u].data(), size);
				}
				printf("\n");
				
				printArray(flat, size*size);
				printf("------------------\n");
	
			}
			
			// if (channel == 0 && i+1 == 587) {
				// for (int u = 0; u < size; u++) {
					// printArray(curChunk[u].data(), size);
				// }
				// printf("\n");
				
				// printArray(flat, size*size);
				// printf("------------------\n");
	
			// }
			
			rleResult = rle::encoder(flat, size);
			
			// if (channel == 0 && i+1 == 1) {
				// for (int u = 0; u < size*size && rleResult[u].first > -1; u++) {
					// printf("(%d, %d)\n", rleResult[u].first, rleResult[u].second);
				// }
				// printf("--------\n");
			// }
			
			populateVector(rleResult, finalVector[channel], size);
			curRleSum = 0;
			free(flat);
		}
		
	}
	
	EntropyEncoded result;
	result.ACComponent = finalVector;
	result.DCComponent = predictedDC;
	
	return result;
	
}


EntropyEncoded EntropyEncode(const ChunkedImage& chunkedImage) {
	
	EntropyEncoded result;
	
	if (chunkedImage.getTransformSpace() == TransformSpace::DCT) {
		result = EntropyEncodeDCT(chunkedImage);
	} 
	
	return result;
	
}


void EntropyDecodeDCT(ChunkedImage& chunkedImage, const EntropyEncoded& encoded) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	
	int *coefficientsDC = NULL;
	std::vector<int> coefficientsAC;
	std::vector<std::pair<int, int>> coefficientsACByChunk(size*size, std::make_pair(-1, -1));
	coefficientsACByChunk[0] = std::make_pair(16, 16);
	std::vector<std::vector<int>> tempArray;
	int j = 0;
	int k = 0;
	int curRleSum = 0;

	std::vector<int *> DCComponent = encoded.DCComponent;
	std::vector<std::vector<std::pair<int, int>>> ACComponent = encoded.ACComponent;
	
	std::vector<std::vector<int>> curChunk;
		
	for (int channel = 0; channel < 3; channel++) {

		coefficientsDC = dpcm::decoder(DCComponent[channel], numChunks, predictionSize);
		k = 0;
		
		for (int i = 0; i < numChunks; i++) {
			chunkedImage.getChunkAt(i)[channel][0][0] = coefficientsDC[i];
		}

		free(coefficientsDC);
		free(DCComponent[channel]);
		
		for (int i = 0; i < numChunks; i++) {
			curChunk = chunkedImage.getChunkAt(i)[channel];

			j = 0;
			curRleSum = 0;
			
			while (curRleSum < size*size-1 && j < size*size-1) {
				coefficientsACByChunk[j+1] = ACComponent[channel][k];
				curRleSum += ACComponent[channel][k].first + 1;
				k++;
				j++;
			}
			
			// printf("TEST!!! sum = %d; j = %d; k = %d, chunkid = %d\n", curRleSum ,j , k, i+1);
			
			
			// if (i+1 == 2) {
				// int a = 0;
				// int b = 0;
				// std::pair<int, int> curRun;
				// while (b < size*size && a < size*size && coefficientsACByChunk[a].first > -1) {
					// curRun = coefficientsACByChunk[a];
					// if (curRun.first > 15 && a == 0 && b == 0) {
						// b++;
						// a++;
					// } else {
						// for (int i = curRun.first; i > 0; i--) {
							// b++;
							// printf("zero found, b = %d\n", b);
						// }
						// b++;
												// printf("val placed, b = %d\n", b);
						// a++;
					// }
					
				// }
				// printf("final b = %d\n", b);	
			// }
			
			coefficientsAC = rle::decoder(coefficientsACByChunk, size);
			tempArray = unflattenArray(coefficientsAC, size);
			populateChunk(tempArray, curChunk, size);
			
			// if (channel == 0 && i+1 == 1772) {
				// printf("\n");
				// for (int u = 0; u < size; u++) {
					// printArray(curChunk[u].data(), size);
				// }
			// }
			
			// if (channel == 0 && i+1 == 587) {
				// printf("\n");
				// for (int u = 0; u < size; u++) {
					// printArray(curChunk[u].data(), size);
				// }
			// }
		}
	}
}

void EntropyDecode(ChunkedImage& chunkedImage, const EntropyEncoded& encoded) {
	
	if (chunkedImage.getTransformSpace() == TransformSpace::DCT) {
		EntropyDecodeDCT(chunkedImage, encoded);
	} 
}

#endif // ENTROPY_HPP