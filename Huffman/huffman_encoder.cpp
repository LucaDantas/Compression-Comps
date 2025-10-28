/*
 * Author: Luca Araujo
 *
 * This program encodes a text file using a precomputed Huffman encoding table.
 * It reads the encoding table and the input text, then writes the encoded bits
 * as bytes to an output file. The last byte contains the number of valid bits
 * in the final byte for correct decoding.
 */

#include <bits/stdc++.h>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <encoding_file> <text_file> <output_file>\n";
        return 1;
    }
    string encodingFile = argv[1];
    string textFile = argv[2];
    string outputFile = argv[3];

    // Step 1: Read Huffman encoding table into array
    string huffmanMap[256];
    ifstream encFile(encodingFile, ios::binary);
    if (!encFile) { cerr << "Error: could not open encoding file\n"; return 1; }
    int character; string code;
    while (encFile >> character >> code) {
        if (character < 0 || character >= 256) {
            cerr << "Error: invalid character in encoding file: " << character << endl;
            return 1;
        }
        huffmanMap[character] = code;
    }
    encFile.close();

    // Step 2: Open original text file
    ifstream textIn(textFile, ios::binary);
    if (!textIn) { cerr << "Error: could not open text file\n"; return 1; }

    // Step 3: Open output binary file
    ofstream outFile(outputFile, ios::binary);
    if (!outFile) { cerr << "Error: could not open output file\n"; return 1; }

    // Step 4: Encode and write bits as bytes
    unsigned char currentByte = 0;
    int bitsFilled = 0;
    char c;
    unsigned char last;
    while (textIn.get(c)) {
        unsigned char uc = c;
        last = uc;
        const string& codeStr = huffmanMap[uc];
        if (codeStr.empty()) {
            cerr << "Error: character not in Huffman map\n";
            return 1;
        }
        // Pack bits into bytes
        for (char bitChar : codeStr) {
            currentByte = (currentByte << 1) | (bitChar - '0');
            bitsFilled++;
            if (bitsFilled == 8) {
                outFile.put(currentByte);
                bitsFilled = 0;
                currentByte = 0;
            }
        }
    }

    // Step 5: Write remaining bits (pad with 0) and footer
    unsigned char validBitsInLastByte = bitsFilled == 0 ? 8 : bitsFilled;
    if (bitsFilled > 0) {
        currentByte <<= (8 - bitsFilled);  // Pad with zeros
        outFile.put(currentByte);
    }
    // Footer: number of valid bits in last byte
    outFile.put(validBitsInLastByte);

    textIn.close();
    outFile.close();
    return 0;
}

