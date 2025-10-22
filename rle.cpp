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
			for (int i = curRun[0]; i > 0; i--) {
				flat[k] = 0;
				k++;
			}
			flat[k] = curRun[1];
			k++;
			j++;
		}
		
		return flat;
	}
	
}

void testRle() {
	
	int size = 8;
	
	printf("Testing encoder...\n");

	int **test = (int **)malloc(sizeof(int *)*size);
	for (int i = 0; i < size; i++) {
		test[i] = (int *)malloc(sizeof(int)*size);
		for (int j = 0; j < size; j++) {
			if ((i+j)%3 == 1 || (i+j)%4 == 2|| (i+j)%7 == 4) {
				test[i][j] = 0;
			} else {
				test[i][j] = (i+1)*(j+1);
			}
		}
		printArray(test[i], 8);
	}
	
	int *flat = zigzagFlattenArray(test, size);
	printArray(flat, 64);	
	
	int **result = rle::encoder(flat, size);
	
	int *last = rle::decoder(result, size);

	printf("Testing decoder...\n");
	
	printArray(last, size*size);
	

	free(last);
	free(flat);
	
	int j = 0;
	while (j < size*size && result[j] != NULL) {
		printf("result[%d] = ", j);
		printArray(result[j], 2);
		free(result[j]);
		j++;
	}
	free(result);
	
		
	for (int i = 0; i < size; i++) {
		free(test[i]);
	}
	free(test);
	
}

// int main(int argc, char **argv) {
	
    // if (argc > 1)
    // {
        // printf("Usage message\n");
        // return 1;
    // }
	
	// testRle();
	
	// return 0;
	
// }