/*
 * Author: Luca Araujo
 *
 * This program builds a Huffman tree and encoding table from a given text file.
 * It reads the file, calculates character frequencies, constructs the Huffman tree,
 * and outputs the encoding table for each character. The tree and encodings are printed
 * for visualization and debugging purposes.
 */

#include <bits/stdc++.h>
using namespace std;

class HuffmanTreeBuilder {
private:
    // Node structure for the Huffman tree: can be a leaf (character) or internal (combination)
    struct Node {
        optional<unsigned char> ch;  // Character (only for leaf nodes)
        int freq;                    // Frequency of the character or subtree
        Node* left;                  // Left child
        Node* right;                 // Right child
        // Constructor for leaf nodes
        Node(unsigned char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
        // Constructor for internal nodes
        Node(int f, Node* l, Node* r) : ch(nullopt), freq(f), left(l), right(r) {}
    } *root = nullptr;               // Root of the Huffman tree
    int freq[256] = {0};             // Frequency table for all ASCII characters
    vector<pair<unsigned char,string>> encodings;  // Stores character -> binary encoding pairs

    // Helper function to print the Huffman tree (for debugging/visualization)
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
        if (node->ch.has_value()) {
            // Print leaf node (character and frequency)
            cerr << "'" << node->ch.value() << "' (" << node->freq << ")" << endl;
        } else {
            // Print internal node (frequency only)
            cerr << "[*] (" << node->freq << ")" << endl;
        }
        printTreeHelper(node->left, indent, true);
        printTreeHelper(node->right, indent, false);
    }

    // Recursively deletes the Huffman tree to free memory
    void deleteTree(Node* node) {
        if (!node) return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

    // Recursive DFS to build binary encodings for each character
    void buildEncodingsDFS(Node* node, string path) {
        if (!node)
            throw runtime_error("incorrect tree trying to be encoded, null non-leaf node found");
        if (node->ch.has_value()) {
            // Leaf node: add the encoding to the table
            encodings.emplace_back(node->ch.value(), path);
            return;
        }
        // Traverse left (add '0') and right (add '1')
        buildEncodingsDFS(node->left, path + "0");
        buildEncodingsDFS(node->right, path + "1");
    }

    // Returns a sorted list of characters by frequency (ascending)
    vector<pair<unsigned char,int>> getSortedFrequencies() const {
        vector<pair<unsigned char,int>> result;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                result.emplace_back((unsigned char)i, freq[i]);
            }
        }
        sort(result.begin(), result.end(),
             [](const pair<unsigned char,int>& a, const pair<unsigned char,int>& b) {
                 return a.second < b.second;
             });
        return result;
    }

    // Builds the Huffman tree using a two queues approach (linear time)
    void buildTree() {
        queue<Node*> characters, combinations;
        auto sortedFreq = getSortedFrequencies();
        if(sortedFreq.size() <= 1)
            throw runtime_error("Text is either empty or only contains one type of character.");

        // Initialize queue with leaf nodes
        for(auto& [ch, f] : sortedFreq)
            characters.push(new Node(ch, f));

        // Helper lambda to get the node with the smallest frequency from both queues
        auto getSmallest = [&]() -> Node* {
            if (characters.empty() && combinations.empty()) {
                return nullptr;
            }
            else if (characters.empty()) {
                Node* node = combinations.front();
                combinations.pop();
                return node;
            }
            else if (combinations.empty()) {
                Node* node = characters.front();
                characters.pop();
                return node;
            }
            else { // There are possibilities to get either a character or a combination, choose the smaller
                if (characters.front()->freq <= combinations.front()->freq) {
                    Node* node = characters.front();
                    characters.pop();
                    return node;
                } else {
                    Node* node = combinations.front();
                    combinations.pop();
                    return node;
                }
            }
        };

        // Main loop, combining nodes until only one remains (the root)
        while(characters.size() + combinations.size() > 1) {
            Node *a = getSmallest();
            Node *b = getSmallest();
            Node *comb = new Node(a->freq + b->freq, a, b);
            combinations.push(comb);
        }
        root = combinations.front();
    }

    // Builds the encoding table by traversing the tree
    void buildEncodings() {
        encodings.clear();
        buildEncodingsDFS(root, "");
    }

public:
    // Destructor: cleans up the tree
    ~HuffmanTreeBuilder() {
        deleteTree(root);
        root = nullptr;
    }

    // Increments the frequency of a character, necessary for building the encoding
    void processChar(unsigned char c) {
        freq[c]++;
    }

    // Prints the frequency table for all characters in the input, used for debugging purposes
    void printFrequencies() const {
        cerr << "\n--- Character Frequency Table ---\n";
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                cerr << "'" << (unsigned char)i << "' (" << i << ") : " << freq[i] << endl;
            }
        }
    }

    // Prints the Huffman tree structure, useful for debugging
    void printTree() const {
        cerr << "\n--- Huffman Tree ---\n";
        printTreeHelper(root, "", false);
    }

    // Main function: builds the tree and returns the encodings
    vector<pair<unsigned char,string>> getEncodings() {
        if(root)
            deleteTree(root); // Ensure only one tree exists at a time
        buildTree();
        buildEncodings();
        return encodings;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_filename> <output_encoding_filename>\n";
        return 1;
    }
    string inputFilename = argv[1];
    string outputEncodingFilename = argv[2];

    ifstream infile(inputFilename, ios::binary);
    if (!infile) {
        cerr << "Error: could not open input file '" << inputFilename << "'" << endl;
        return 1;
    }

    HuffmanTreeBuilder htb;
    try {
        // Read file and process frequencies
        char c;
        while (infile.get(c))
            htb.processChar((unsigned char) c);
        infile.close();

        // // Print character frequencies to stderr
        // htb.printFrequencies();

        // Retrieve encodings
        auto table = htb.getEncodings();

        // // Print encodings to stderr
        // cerr << "\n--- Huffman Encodings ---\n";
        // for (auto& [ch, code] : table)
        //     cerr << "'" << (int) ch << "' : " << code << endl;

        // Write encodings to output file
        ofstream outEncodingFile(outputEncodingFilename);
        if (!outEncodingFile) {
            cerr << "Error: could not open output encoding file '" << outputEncodingFilename << "'" << endl;
            return 1;
        }
        for (auto& [ch, code] : table)
            outEncodingFile << (int)(ch) << " " << code << endl;
        outEncodingFile.close();

        // // Print the tree (for visualization)
        // htb.printTree();
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}


