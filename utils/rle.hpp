#ifndef RLE_HPP
#define RLE_HPP

#include <stdlib.h>
#include <stdio.h>
// #include "utils/image_lib.hpp"
#include "dpcm.hpp"

namespace rle {
	
	std::vector<std::pair<int, int>> encoder(int *flat, int size) {
		
		std::vector<std::pair<int, int>> result(size*size, std::make_pair(-1, -1)); //2d array, result[i] = (# preceding zeroes, n)
		
		int j = 0;
		std::pair<int, int> curRun = std::make_pair(0, 0);
		
		for (int i = 1; i < size*size; i++) {
			
			if (i < size*size-1 && flat[i] == 0 && curRun.first < 15) {
				curRun.first += 1;
			} else {
				curRun.second = flat[i];
				result[j] = curRun;
				curRun = std::make_pair(0, 0);
				j++;
			}
			
		}
		
		if (j < size*size-1) {
			result[j] = std::make_pair(-1, -1);
		}
		
		return result;
	}
	
	std::vector<int> decoder(const std::vector<std::pair<int, int>>& arr, int size) {
		
		std::vector<int> flat(size*size, 0);
		std::pair<int, int> curRun;
		
		int j = 0;
		int k = 0; // flat counter

		while (k < size*size && j < size*size && arr[j].first > -1) {
			curRun = arr[j];
			if (curRun.first > 15 && j == 0 && k == 0) { // check for throwaway DC pair at chunk[0][0]
				flat[k] = -1;
				k++;
				j++;
			} else {
				for (int i = curRun.first; i > 0; i--) {
					flat[k] = 0;
					k++;
				}
				flat[k] = curRun.second;
				k++;
				j++;
			}
		}
		
		return flat;
	}
	
}

#endif // RLE_HPP