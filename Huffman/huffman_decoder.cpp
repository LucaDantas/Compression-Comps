/*
 * Author: Luca Araujo
 *
 * Overview:
 * This file implements a Huffman decoder. It reads a Huffman encoding table and a binary
 * encoded file, then reconstructs the original text by traversing the Huffman tree
 * according to the encoded bits. The last byte of the input file specifies the number
 * of valid bits in the final byte.
 */

#include <bits/stdc++.h>
using namespace std;

// Node structure for the Huffman tree
struct HuffmanNode {
    optional<char> ch;          // Character (only for leaf nodes)
    HuffmanNode* left = nullptr;
    HuffmanNode* right = nullptr;
    HuffmanNode(optional<char> c = nullopt) : ch(c), left(nullptr), right(nullptr) {}
};

/**
 * Builds a Huffman tree from an encoding table.
 * @param huffmanMap  Array where huffmanMap[i] is the binary code for ASCII character i.
 * @return Pointer to the root of the constructed Huffman tree.
 * @throws runtime_error If the encoding table contains invalid or duplicate codes.
 */
HuffmanNode* buildHuffmanTree(const string huffmanMap[256]) {
    HuffmanNode* root = new HuffmanNode();
    for (int i = 0; i < 256; ++i) {
        const string& code = huffmanMap[i];
        if (code.empty()) continue;
        HuffmanNode* node = root;
        for (char bit : code) {
            if (bit == '0') {
                if (!node->left) node->left = new HuffmanNode();
                node = node->left;
            } else if (bit == '1') {
                if (!node->right) node->right = new HuffmanNode();
                node = node->right;
            } else {
                throw runtime_error("Invalid bit in Huffman code");
            }
        }
        if (node->ch.has_value())
            throw runtime_error("Duplicate Huffman code in map");
        node->ch = static_cast<char>(i);
    }
    return root;
}

// Recursively deletes the Huffman tree to free memory
void deleteHuffmanTree(HuffmanNode* node) {
    if (!node) return;
    deleteHuffmanTree(node->left);
    deleteHuffmanTree(node->right);
    delete node;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <encoding_file> <encoded_file> <output_file>\n";
        return 1;
    }
    string encodingFile = argv[1];
    string encodedFile = argv[2];
    string outputFile = argv[3];

    // --- Step 1: read Huffman encoding ---
    string huffmanMap[256];
    ifstream encFile(encodingFile);
    if (!encFile) { cerr << "Cannot open encoding file\n"; return 1; }
    int ascii; string code;
    while (encFile >> ascii >> code) huffmanMap[ascii] = code;
    encFile.close();

    // --- Step 2: build Huffman tree ---
    HuffmanNode* root = buildHuffmanTree(huffmanMap);

    // --- Step 3: open encoded file ---
    ifstream encodedIn(encodedFile, ios::binary);
    if (!encodedIn) { cerr << "Cannot open encoded file\n"; deleteHuffmanTree(root); return 1; }
    vector<unsigned char> dataBytes;
    char c;
    while (encodedIn.get(c)) dataBytes.push_back(static_cast<unsigned char>(c));
    encodedIn.close();
    if (dataBytes.empty()) { cerr << "Encoded file is empty\n"; deleteHuffmanTree(root); return 1; }

    // --- Step 4: last byte is footer (number of valid bits in last byte) ---
    int validBitsInLastByte = dataBytes.back();
    dataBytes.pop_back(); // remove footer

    // --- Step 5: open output file ---
    ofstream outFile(outputFile, ios::binary);
    if (!outFile) { cerr << "Cannot open output file\n"; deleteHuffmanTree(root); return 1; }

    // --- Step 6: decode bit by bit ---
    HuffmanNode* node = root;
    for (size_t i = 0; i < dataBytes.size(); ++i) {
        unsigned char byte = dataBytes[i];
        int bitsPadding = (i == dataBytes.size() - 1) ? 8-validBitsInLastByte : 0;
        for (int j = 7; j >= bitsPadding; --j) {
            bool bit = (byte >> j) & 1;
            node = bit ? node->right : node->left;
            if (!node) { cerr << "Error: reached null node in tree\n"; deleteHuffmanTree(root); return 1; }
            if (node->ch.has_value()) {
                outFile.put(node->ch.value());
                node = root; // reset for next character
            }
        }
    }

    outFile.close();
    deleteHuffmanTree(root);
    return 0;
}

