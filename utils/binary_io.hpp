#ifndef BINARY_IO_HPP
#define BINARY_IO_HPP

#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

void writeVectorToFile(const std::vector<int>& data, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    
    // Write the size of the vector first
    size_t size = data.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    
    // Write the vector data
    if (size > 0) {
        file.write(reinterpret_cast<const char*>(data.data()), size * sizeof(int));
    }
    
    file.close();
}

std::vector<int> readVectorFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filename);
    }
    
    // Read the size of the vector first
    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
    
    if (!file) {
        throw std::runtime_error("Failed to read size from file: " + filename);
    }
    
    // Read the vector data
    std::vector<int> data(size);
    if (size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), size * sizeof(int));
        if (!file) {
            throw std::runtime_error("Failed to read data from file: " + filename);
        }
    }
    
    file.close();
    return data;
}

#endif

