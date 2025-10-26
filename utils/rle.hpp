#ifndef RLE_HPP
#define RLE_HPP

#include <stdlib.h>
#include <stdio.h>
// #include "utils/image_lib.hpp"
#include "dpcm.hpp"

namespace rle {
	
	int **encoder(int *flat, int size) {
		
		int **result = (int **)malloc(sizeof(int *)*size*size); //2d array, result[i] = (# preceding zeroes, n)
		int j = 0;
		int *curRun = (int *)malloc(sizeof(int)*2);
		curRun[0] = 0;
		curRun[1] = 0;
		
		for (int i = 0; i < size*size; i++) {
			
			if (i < size*size-1 && flat[i] == 0 && curRun[0] < 15) {
				curRun[0] += 1;
			} else {
				curRun[1] = flat[i];
				result[j] = curRun;
				curRun = (int *)malloc(sizeof(int)*2);
				curRun[0] = 0;
				curRun[1] = 0;
				j++;
			}
			
		}
		
		if (j < size*size - 1) {
			result[j] = NULL;
		}
		
		return result;
	}
	
	int *decoder(int **arr, int size) {
		
		int *flat = (int *)malloc(sizeof(int)*size*size);
		int *curRun = NULL;
		
		int j = 0;
		int k = 0; // flat counter
		while (j < size*size && arr[j] != NULL) {
			curRun = arr[j];
			if (curRun[0] > 15 && j == 0 && k == 0){
				flat[k] = -1;
				k++;
				j++;
			} else {
				for (int i = curRun[0]; i > 0; i--) {
					flat[k] = 0;
					k++;
				}
				flat[k] = curRun[1];
				k++;
				j++;
			}
		}
		
		return flat;
	}
	
}

#endif // RLE_HPP