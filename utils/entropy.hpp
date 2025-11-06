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
std::vector<int> EntropyEncode(const ChunkedImage& chunkedImage);
void EntropyDecodeDCT(ChunkedImage& chunkedImage, const EntropyEncoded& encoded);
void EntropyDecode(ChunkedImage& chunkedImage, const EntropyEncoded& encoded);

void populateVector(const std::vector<std::pair<int, int>>& arr, std::vector<std::pair<int, int>>& targetVector, int size) {
	
		int j = 0;
		while (j < size*size-1 && arr[j].first > -1) {
			targetVector.push_back(arr[j]);
			j++;
		}
	
}

void populateChunk(const std::vector<std::vector<int>>& arr, std::vector<std::vector<int>>* chunk, int size) {
	
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (i == 0 && j == 0) {
				continue;
			} else {
				(*chunk)[i][j] = arr[i][j];
			}
		}
	}
}

void populateChunkFull(const std::vector<std::vector<int>>& arr, std::vector<std::vector<int>>* chunk, int size) {
	
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			(*chunk)[i][j] = arr[i][j];
		}
	}
}

std::vector<int> EntropyEncodeToVec(const EntropyEncoded& encoded, int numChunks, int size) { // 0 - numChunks; 1 - size; 2 - transformSpace;
	std::vector<int> result;
	result.push_back(numChunks);
	result.push_back(size);
	result.push_back(0);
	
	std::vector<int *> DCComponent = encoded.DCComponent;
	std::vector<std::vector<std::pair<int, int>>> ACComponent = encoded.ACComponent;
	
	for (int channel = 0; channel < 3; channel++) {
		
		result.insert(result.end(), DCComponent[channel], DCComponent[channel] + numChunks);
		
		free(DCComponent[channel]);
		
		result.push_back(ACComponent[channel].size());
		
		for (int i = 0; i < ACComponent[channel].size(); i++) {
			result.push_back(ACComponent[channel][i].first);
			result.push_back(ACComponent[channel][i].second);
		}
		
	}
	
	return result;
	
}

void VecToEntropyEncode(std::vector<int> encoded, EntropyEncoded& encodedDCT, int numChunks, int size) {
	
	int *tempDC;
	
	int i = 3; //ignore metadata
	int ACSize;
	
	std::vector<int *> DCComponent(3, NULL);
	std::vector<std::vector<std::pair<int, int>>> ACComponent(3, std::vector<std::pair<int, int>>());
	
	for (int channel = 0; channel < 3; channel++) {
		
		// DC
		
		tempDC = (int *)malloc(sizeof(int)*numChunks);
		
		for (int j = 0; j < numChunks; j++) {
			tempDC[j] = encoded[j+i];
		}
		
		DCComponent[channel] = tempDC;
		
		// AC 
		
		i += numChunks;
		ACSize = encoded[i];
		i++;
		
		for (int j = 0; j < 2*ACSize; j += 2) {
			ACComponent[channel].push_back(std::make_pair(encoded[i+j], encoded[i+j+1]));
		}
		
		i += 2*ACSize;
		
	}
	
	encodedDCT.DCComponent = DCComponent;
	encodedDCT.ACComponent = ACComponent;
	
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
			
			rleResult = rle::encoder(flat, size);

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

std::vector<int> EntropyEncodeHaar(const ChunkedImage& chunkedImage) {
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	int *predictedResult = NULL;
	const std::vector<int> *curChunk = NULL;
	std::vector<int> result;
	result.push_back(numChunks);
	result.push_back(size);
	TransformSpace transformSpace = chunkedImage.getTransformSpace();
    switch (transformSpace) {
        case TransformSpace::Haar: 
			result.push_back(1);
			break;
        case TransformSpace::SP:
			result.push_back(2);
			break;
		default:
			break;
    }
	int *flat;
	
	for (int channel = 0; channel < 3; channel++) {
		for (int i = 0; i < numChunks; i++) {
			curChunk = chunkedImage.getChunkAt(i)[channel].data();
			flat = zigzagFlattenArray(curChunk, size);
			
			predictedResult = dpcm::encoder(flat, size*size, predictionSize);
			free(flat);
			
			result.insert(result.end(), predictedResult, predictedResult + size*size);
			free(predictedResult);
		}
	}

	return result;
	
}


std::vector<int> EntropyEncode(const ChunkedImage& chunkedImage) {
	
	std::vector<int> result;
	
	if (chunkedImage.getTransformSpace() == TransformSpace::DCT) {
		EntropyEncoded resultInitial;
		resultInitial = EntropyEncodeDCT(chunkedImage);
		result = EntropyEncodeToVec(resultInitial, chunkedImage.getTotalChunks(), chunkedImage.getChunkSize());
	} else if (chunkedImage.getTransformSpace() == TransformSpace::SP || chunkedImage.getTransformSpace() == TransformSpace::Haar) {
		result = EntropyEncodeHaar(chunkedImage);
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
	
	std::vector<std::vector<int>> *curChunk;
		
	for (int channel = 0; channel < 3; channel++) {

		coefficientsDC = dpcm::decoder(DCComponent[channel], numChunks, predictionSize);
		k = 0;
		
		for (int i = 0; i < numChunks; i++) {
			chunkedImage.getChunkAt(i)[channel][0][0] = coefficientsDC[i];
		}

		free(coefficientsDC);
		free(DCComponent[channel]);
		
		for (int i = 0; i < numChunks; i++) {
			curChunk = &chunkedImage.getChunkAt(i)[channel];

			j = 0;
			curRleSum = 0;
			
			while (curRleSum < size*size-1 && j < size*size-1) {
				coefficientsACByChunk[j+1] = ACComponent[channel][k];
				curRleSum += ACComponent[channel][k].first + 1;
				k++;
				j++;
			}
			
			coefficientsAC = rle::decoder(coefficientsACByChunk, size);
			tempArray = unflattenArray(coefficientsAC, size);
			populateChunk(tempArray, curChunk, size);
			
		}
	}
}

void EntropyDecodeHaar(ChunkedImage& chunkedImage, std::vector<int> encoded) {
	
	int numChunks = chunkedImage.getTotalChunks();
	int size = chunkedImage.getChunkSize();
	int predictionSize = 4;
	int *encodedFlat = NULL;
	int *decodedFlat = NULL;
	std::vector<std::vector<int>> tempArray;
	std::vector<std::vector<int>> *curChunk;
	
	for (int channel = 0; channel < 3; channel++) {
		for (int i = 0; i < numChunks; i++) {
			curChunk = &chunkedImage.getChunkAt(i)[channel];
			
			encodedFlat = encoded.data() + 3 + channel*numChunks*size*size + i*size*size;
			decodedFlat = dpcm::decoder(encodedFlat, size*size, predictionSize);
			tempArray = unflattenArray(std::vector<int>(decodedFlat, decodedFlat + size*size), size);
			free(decodedFlat);
			populateChunkFull(tempArray, curChunk, size);
		}	
	}
}

void EntropyDecode(ChunkedImage& chunkedImage, std::vector<int> encoded) {
	
	if (chunkedImage.getTransformSpace() == TransformSpace::DCT) {
		EntropyEncoded encodedDCT;
		VecToEntropyEncode(encoded, encodedDCT, chunkedImage.getTotalChunks(), chunkedImage.getChunkSize());
		EntropyDecodeDCT(chunkedImage, encodedDCT);
	} else if (chunkedImage.getTransformSpace() == TransformSpace::Haar || chunkedImage.getTransformSpace() == TransformSpace::SP) {
		EntropyDecodeHaar(chunkedImage, encoded);

	}
}

#endif // ENTROPY_HPP