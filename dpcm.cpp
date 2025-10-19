#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "dpcm.hpp"
#include "utils/image_lib.hpp"

void printArray(int *arr, int size) {
	
	printf("[ ");
	
	for (int i = 0; i < size; i++) {
		printf("%d ", arr[i]);
	}
	
	printf("]\n");
	
}

int sum(int *arr, int size) {
	
	int total = 0;
	
	for (int i = 0; i < size; i++) {
		total += arr[i];
	}
	
	return total;
}

int dotProduct(int *arr1, int *arr2, int size) {
	
	int total = 0;
	
	for (int i = 0; i < size; i++) {
		total += arr1[i] * arr2[i];
	}
	
	return total;
	
}

int *createLinearArray(int size) {
	
	int *arr = (int *)malloc(sizeof(int)*size);
	
	for (int i = 0; i < size; i++) {
		arr[i] = i+1;
	}
	
	return arr;
	
}

int *zigzagFlattenArray(int **arr, int size) {
	
	int i = 0;
	int j = 0;
	int k = 0; // flat
	int val;
	int ste;
	int dir = 1;
	int sign = 1;
	
	int *flat = (int *)malloc(sizeof(int)*size*size);
	
	for (int rep = -7; rep < 8; rep++) {
		ste = 7 - abs(rep);
		
		while (ste >= 0) {
			flat[k] = arr[i][j];
			
			i += dir;
			j -= dir;
			k++;
			ste--;
		}
		
		i -= dir;
		j += dir;
		
		if (rep == 0) {
			sign = -1;
		}
		
		if (dir*sign > 0) {
			i += 1;
		} else {
			j += 1;
		}
		
		dir *= -1;
	}
	
	return flat;
}

int **unflattenArray(int *arr, int size) {
	
	int i = 0;
	int j = 0;
	int k = 0; // flat
	int val;
	int ste;
	int dir = 1;
	int sign = 1;
	
	int **chunk = (int **)malloc(sizeof(int *)*size);
	for (int i = 0; i < 8; i++) {
		chunk[i] = (int *)malloc(sizeof(int)*8);
	}
	
	for (int rep = -7; rep < 8; rep++) {
		ste = 7 - abs(rep);
		
		while (ste >= 0) {
			chunk[i][j] = arr[k];
			
			i += dir;
			j -= dir;
			k++;
			ste--;
		}
		
		i -= dir;
		j += dir;
		
		if (rep == 0) {
			sign = -1;
		}
		
		if (dir*sign > 0) {
			i += 1;
		} else {
			j += 1;
		}
		
		dir *= -1;
	}
	
	return chunk;
}

int linearPredictor(int *y_arr, int size) {
	
	int *x_arr = createLinearArray(size);
	
	int x_sum = sum(x_arr, size);
	int y_sum = sum(y_arr, size);
	int x_squared_sum = dotProduct(x_arr, x_arr, size);
	int xy_sum = dotProduct(x_arr, y_arr, size);
	
	double b; // y = mx + b
	double m;
	
	b = (double)(y_sum*x_squared_sum - x_sum*xy_sum) / (size*x_squared_sum - x_sum*x_sum);
	m = (double)(size*xy_sum - x_sum*y_sum) / (size*x_squared_sum - x_sum*x_sum);
	
	// printArray(y_arr, size);
	// printf("b = %f\n", b);
	// printf("m = %f\n", m);
	
	int next_x = size+1;
	double next_y_d = m*next_x + b;
	int next_y = (int)next_y_d;
	
	// printf("next_y = %d\n", next_y);
	
	free(x_arr);
	
	return next_y;
	
}
namespace dpcm {
	int *encoder(int *arr, int size, int prediction_size) {
		
		int *result = (int *)malloc(sizeof(int)*size*size);
		int *sample = (int *)malloc(sizeof(int)*prediction_size);
		
		int i = 0;
		
		while (i < prediction_size) {
			result[i] = arr[i];
			i++;
		}
		
		while (i < size*size) {
			for (int j = 0; j < prediction_size; j++) {
				sample[j] = arr[i - (prediction_size-j)];
			}
			
			result[i] = arr[i] - linearPredictor(sample, prediction_size);
			i++;
		}
		
		free(sample);
		return result;
	}

	int **decoder(int *arr) {
		int size = 8;
		int prediction_size = 4;
		
		int *flattened_chunk = (int *)malloc(sizeof(int)*size*size);
		int *sample = (int *)malloc(sizeof(int)*prediction_size);
		
		int i = 0;
		
		while (i < prediction_size) {
			flattened_chunk[i] = arr[i];
			i++;
		}	
		
		while (i < size*size) {
			for (int j = 0; j < prediction_size; j++) {
				sample[j] = flattened_chunk[i - (prediction_size-j)];
			}
			
			flattened_chunk[i] = arr[i] + linearPredictor(sample, prediction_size);
			i++;
		}
		
		int **result = unflattenArray(flattened_chunk, size);
		
		free(sample);
		free(flattened_chunk);
		return result;
	}
}
void test_flatten() {
	
	printf("Testing flatten...\n");
	
	int **test = (int **)malloc(sizeof(int *)*8);
	for (int i = 0; i < 8; i++) {
		test[i] = (int *)malloc(sizeof(int)*8);
		for (int j = 0; j < 8; j++) {
			test[i][j] = (i+1)*(j+1);
		}
		printArray(test[i], 8);
	}
	
	int *flat = zigzagFlattenArray(test, 8);
	printArray(flat, 64);
	
	printf("Testing unflatten...\n");
	
	int **chunk = unflattenArray(flat, 8);
	
	free(flat);
	
	for (int i = 0; i < 8; i++) {
		free(test[i]);
	}
	free(test);
	
	for (int i = 0; i < 8; i++) {
		printArray(chunk[i], 8);
		free(chunk[i]);
	}
	free(chunk);
	
}

void test_linear() {
	
	printf("Testing linear predictor...\n");
	
	int *test = (int *)malloc(sizeof(int)*4);
	for (int i = 0; i < 4; i ++) {
		test[i] = (i+1)*12;
	}
	
	printArray(test, 4);
	
	int next = linearPredictor(test, 4);
	printf("next = %d\n", next);
}

void test_encoder() {
	
	printf("Testing encoder...\n");
	
	int **test = (int **)malloc(sizeof(int *)*8);
	for (int i = 0; i < 8; i++) {
		test[i] = (int *)malloc(sizeof(int)*8);
		for (int j = 0; j < 8; j++) {
			test[i][j] = (i+1)*(j+1);
		}
		printArray(test[i], 8);
	}
	
	int *encoded = dpcm::encoder(test);
	printArray(encoded, 64);
	
	printf("Testing decoder...\n");
	
	int **decoded = dpcm::decoder(encoded);
	
	free(encoded);
	
	for (int i = 0; i < 8; i++) {
		printArray(decoded[i], 8);
		free(decoded[i]);
	}
	
	free(decoded);
	
	for (int i = 0; i < 8; i++) {
		free(test[i]);
	}
	free(test);
}

int main(int argc, char **argv) {
	
    if (argc > 1)
    {
        printf("Usage message\n");
        return 1;
    }
	
	// test_flatten();
	// test_linear();
	// test_encoder();
	
	return 0;
	
}