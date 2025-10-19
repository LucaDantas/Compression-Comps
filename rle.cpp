#include <stdlib.h>
#include <stdio.h>
#include "utils/image_lib.hpp"
#include "dpcm.hpp"

namespace rle {
	
	int **encoder(int *flat, int size) {
		
		int **result = (int **)malloc(sizeof(int *)*size*size); //2d array, result[i] = (# preceding zeroes, n)
		int j = 0;
		int precedingZeroes = 0;
		int *curRun = (int *)malloc(sizeof(int)*2);
		curRun[0] = 0;
		curRun[1] = 0;
		
		for (int i = 0; i < size*size; i++) {
			
			if (flat[i] == 0 && curRun[0] < 15) {
				curRun[0] += 1;
			} else {
				curRun[1] = flat[i];
				result[j] = curRun;
				int *curRun = (int *)malloc(sizeof(int)*2);
				curRun[0] = 0;
				curRun[1] = 0;
				j++;
			}
			
		}
		
		return NULL;
	}
	
	int *decoder(int **arr, int size) {
		
		return NULL;
	}
	
}

int main(int argc, char **argv) {
	
    if (argc > 1)
    {
        printf("Usage message\n");
        return 1;
    }
	
	
	
	return 0;
	
}