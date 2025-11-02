#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include <bits/stdc++.h>
using namespace std;

// Output of the encoding is a encoding of the tree and then a list of values for each leaf
class HuffmanTree {
private:
    struct Node {
        optional<int> value;
        Node* left;
        Node* right;
        Node(int v) : value(v), left(nullptr), right(nullptr) {}
        Node(Node* l, Node* r) : value(nullopt), left(l), right(r) {}
    } *root = nullptr;

    void printTreeHelper(Node* node, string indent = "", bool isLeft = true) const {
        if (!node) return;
        cerr << indent;
        if (isLeft) {
            cerr << "├──";
            indent += "│   ";
        } else {
            cerr << "└──";
            indent += "    ";
        }
        if (node->value.has_value()) {
            cerr << node->value.value() << endl;
        } else {
            cerr << "[*]" << endl;
        }
        printTreeHelper(node->left, indent, true);
        printTreeHelper(node->right, indent, false);
    }

    void deleteTree(Node* node) {
        if (!node) return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

public:
    HuffmanTree(const map<int, int>& freq) {
        queue<pair<int, Node*>> characters, combinations;
        
        auto getSortedFrequencies = [&freq]() -> vector<pair<int,int>> {
            vector<pair<int,int>> result;
            for (const auto& [value, frequency] : freq) {
                result.emplace_back(value, frequency);
            }
            sort(result.begin(), result.end(),
                 [](const pair<int,int>& a, const pair<int,int>& b) {
                     return a.second < b.second;
                 });
            return result;
        };
        
        auto sortedFreq = getSortedFrequencies();

        if(sortedFreq.size() <= 1)
            throw runtime_error("Input is either empty or only contains one type of integer value.");

        for(auto& [val, f] : sortedFreq)
            characters.push({f, new Node(val)});

        auto getSmallest = [&characters, &combinations]() -> pair<int, Node*> {
            if (characters.empty() && combinations.empty()) {
                return {0, nullptr};
            }
            else if (characters.empty()) {
                pair<int, Node*> p = combinations.front();
                combinations.pop();
                return p;
            }
            else if (combinations.empty()) {
                pair<int, Node*> p = characters.front();
                characters.pop();
                return p;
            }
            else {
                if (characters.front().first <= combinations.front().first) {
                    pair<int, Node*> p = characters.front();
                    characters.pop();
                    return p;
                } else {
                    pair<int, Node*> p = combinations.front();
                    combinations.pop();
                    return p;
                }
            }
        };

        while(characters.size() + combinations.size() > 1) {
            auto [freq_a, a] = getSmallest();
            auto [freq_b, b] = getSmallest();
            Node *comb = new Node(a, b);
            combinations.push({freq_a + freq_b, comb});
        }
        root = combinations.front().second;
    }

    HuffmanTree(const vector<bool>& dfsPath, const vector<int>& leafValues) {
        int posDfs = 0, posLeaf = 0;

        function<Node*(void)> buildTreeFromEncodingRecursive = [&]() -> Node* {
            if(dfsPath[posDfs++] == 0) { // internal node
                Node* leftNode = buildTreeFromEncodingRecursive();
                Node* rightNode = buildTreeFromEncodingRecursive();
                return new Node(leftNode, rightNode);
            } else { // leaf
                return new Node(leafValues[posLeaf++]);
            }
        };

        root = buildTreeFromEncodingRecursive();
    }

    ~HuffmanTree() {
        deleteTree(root);
        root = nullptr;
    }

    void printTree() const {
        cerr << "\n--- Huffman Tree ---\n";
        printTreeHelper(root, "", false);
    }

    map<int, string> getValueEncodings() {
        map<int, string> result;
        
        function<void(Node*, string)> buildValueEncodingsRecursive = [&](Node* node, string path) -> void {
            if (!node)
                throw runtime_error("incorrect tree trying to be encoded, null non-leaf node found");
            if (node->value.has_value()) {
                result[node->value.value()] = path;
                return;
            }
            buildValueEncodingsRecursive(node->left, path + "0");
            buildValueEncodingsRecursive(node->right, path + "1");
        };
        
        buildValueEncodingsRecursive(root, "");
        return result;
    }

    pair<vector<bool>, vector<int>> getTreeEncoding() {
        vector<bool> dfsPath;
        vector<int> leafValues;
        
        function<void(Node*)> buildTreeEncodingRecursive = [&](Node* node) -> void {
            if (node->value.has_value()) {
                dfsPath.push_back(1);
                leafValues.push_back(node->value.value());
                return;
            }
            dfsPath.push_back(0); // exploit the property that it always has two children
            buildTreeEncodingRecursive(node->left);
            buildTreeEncodingRecursive(node->right);
        };
        
        buildTreeEncodingRecursive(root);
        return make_pair(dfsPath, leafValues);
    }

    vector<int> decodeBinaryString(const string& binaryString) const {
        vector<int> result;
        Node* current = root;
        
        if (!root) {
            throw runtime_error("Cannot decode: tree is empty");
        }
        
        for (char bit : binaryString) {
            if (bit == '0') {
                current = current->left;
            } else if (bit == '1') {
                current = current->right;
            } else {
                throw runtime_error("Invalid character in binary string: only '0' and '1' allowed");
            }
            
            if (!current) {
                throw runtime_error("Invalid binary string: reached null node");
            }
            
            if (current->value.has_value()) {
                result.push_back(current->value.value());
                current = root;  // Reset to root after reaching a leaf
            }
        }
        
        if (current != root) {
            throw runtime_error("Invalid binary string: not all bits yielded complete symbols");
        }
        
        return result;
    }
};

class HuffmanEncoder {
private:
    map<int, int> computeFrequencyMap(const vector<int>& values) const {
        map<int, int> freq;
        for (int value : values) {
            freq[value]++;
        }
        return freq;
    }

    vector<int> boolVectorToBitmask(const vector<bool>& boolVec) const {
        vector<int> result;
        int currentInt = 0;
        int bitIndex = 0;
        
        for (bool bit : boolVec) {
            if (bit) {
                currentInt |= (1 << (31 - bitIndex));
            }
            bitIndex++;
            
            if (bitIndex == 32) {
                result.push_back(currentInt);
                currentInt = 0;
                bitIndex = 0;
            }
        }
        
        if (bitIndex > 0) {
            result.push_back(currentInt);
        }
        
        return result;
    }

    vector<int> stringToBitmask(const string& bits) const {
        vector<int> result;
        int currentInt = 0;
        int bitIndex = 0;
        
        for (char bitChar : bits) {
            if (bitChar == '1') {
                currentInt |= (1 << (31 - bitIndex));
            }
            bitIndex++;
            
            if (bitIndex == 32) {
                result.push_back(currentInt);
                currentInt = 0;
                bitIndex = 0;
            }
        }
        
        if (bitIndex > 0) {
            result.push_back(currentInt);
        }
        
        // Append the number of empty bits in the last int used for encoding
        // If bitIndex == 0, the last int was completely filled, so 0 empty bits
        // Otherwise, bitIndex is the number of bits used, so (32 - bitIndex) are empty
        int emptyBits = (bitIndex == 0) ? 0 : (32 - bitIndex);
        result.push_back(emptyBits);
        
        return result;
    }

public:
    HuffmanEncoder() {}

    vector<int> getEncoding(const vector<int>& values) const {
        // Build Huffman tree and get encodings
        HuffmanTree tree(computeFrequencyMap(values));
        map<int, string> encodingMap = tree.getValueEncodings();
        pair<vector<bool>, vector<int>> treeEncoding = tree.getTreeEncoding();
        
        vector<int> result;
        
        // Convert tree encoding dfsPath to bitmasks
        const vector<bool>& dfsPath = treeEncoding.first;
        vector<int> dfsPathBitmask = boolVectorToBitmask(dfsPath);
        
        // Add size of tree encoding (number of ints needed for dfsPath)
        result.push_back(dfsPathBitmask.size());
        
        // Add tree encoding dfsPath as bitmasks
        result.insert(result.end(), dfsPathBitmask.begin(), dfsPathBitmask.end());
        
        // Add leaf values (already ints)
        result.insert(result.end(), treeEncoding.second.begin(), treeEncoding.second.end());
        
        // Get text encoding string and convert to bitmasks
        string textEncodingString;
        for (int value : values) {
            textEncodingString += encodingMap.at(value);
        }
        vector<int> textBitmask = stringToBitmask(textEncodingString);
        result.insert(result.end(), textBitmask.begin(), textBitmask.end());
        
        return result;
    }
};

class HuffmanDecoder {
private:
    vector<bool> bitmaskToBoolVector(const vector<int>& bitmasks) const {
        vector<bool> result;
        
        for (int mask : bitmasks) {
            for (int i = 31; i >= 0; i--) {
                result.push_back((mask >> i) & 1);
            }
        }
        
        return result;
    }

    string bitmaskToString(const vector<int>& bitmasks, int emptyBitsInLastInt = 0) const {
        if (bitmasks.empty()) {
            return "";
        }
        
        string result;
        
        // Process all but the last int (full 32 bits each)
        for (size_t i = 0; i < bitmasks.size() - 1; i++) {
            int mask = bitmasks[i];
            for (int j = 31; j >= 0; j--) {
                result += ((mask >> j) & 1) ? '1' : '0';
            }
        }
        
        // Process the last int, skipping the empty bits
        if (!bitmasks.empty()) {
            int lastMask = bitmasks.back();
            int bitsToProcess = 32 - emptyBitsInLastInt;
            for (int j = 31; j >= (32 - bitsToProcess); j--) {
                result += ((lastMask >> j) & 1) ? '1' : '0';
            }
        }
        
        return result;
    }

public:
    HuffmanDecoder() {}

    vector<int> decode(const vector<int>& encoded) const {
        if (encoded.empty()) {
            throw runtime_error("Encoded data is empty");
        }
        
        size_t index = 0;
        
        // Read the number of ints in dfsPathBitmask
        int dfsPathBitmaskSize = encoded[index++];
        
        if (dfsPathBitmaskSize < 0 || index + dfsPathBitmaskSize > encoded.size()) {
            throw runtime_error("Invalid dfsPath bitmask size");
        }
        
        // Read the dfsPath bitmasks
        vector<int> dfsPathBitmasks;
        for (int i = 0; i < dfsPathBitmaskSize; i++) {
            dfsPathBitmasks.push_back(encoded[index++]);
        }
        
        // Convert bitmasks back to vector<bool>
        vector<bool> dfsPath = bitmaskToBoolVector(dfsPathBitmasks);
        
        // Remove trailing zeros until the last 1
        while (!dfsPath.empty() && dfsPath.back() == false) {
            dfsPath.pop_back();
        }
        
        if (dfsPath.empty()) {
            throw runtime_error("Invalid dfsPath: no bits found");
        }
        
        // Count the number of 1s in dfsPath
        int numOnes = 0;
        for (bool bit : dfsPath) {
            if (bit) {
                numOnes++;
            }
        }
        
        // Read that many leaf values
        if (index + numOnes > encoded.size()) {
            throw runtime_error("Not enough data for leaf values");
        }
        
        vector<int> leafValues;
        for (int i = 0; i < numOnes; i++) {
            leafValues.push_back(encoded[index++]);
        }
        
        // Create HuffmanTree from the tree encoding
        HuffmanTree tree(dfsPath, leafValues);
        
        // Get the remaining encoded data (text encoding bitmasks)
        // The last element is the number of empty bits in the last int
        if (index >= encoded.size() - 1) {
            throw runtime_error("Not enough data for text encoding bitmasks");
        }
        
        // Extract text bitmasks (all but the last element which is the padding count)
        vector<int> textBitmasks(encoded.begin() + index, encoded.end() - 1);
        int emptyBitsInLastInt = encoded.back();
        
        // Validate empty bits count
        if (emptyBitsInLastInt < 0 || emptyBitsInLastInt > 32) {
            throw runtime_error("Invalid empty bits count in last int");
        }
        
        // Convert bitmasks to binary string
        string binaryString = bitmaskToString(textBitmasks, emptyBitsInLastInt);
        
        // Decode using the tree
        return tree.decodeBinaryString(binaryString);
    }
};

#endif