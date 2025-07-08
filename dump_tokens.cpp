#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open source file: " << filename << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    std::cout << "Token dump for " << filename << ":" << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    // Show tokens around position 196
    int target = 196;
    int start = std::max(0, target - 10);
    int end = std::min(static_cast<int>(tokens.size()), target + 10);
    
    for (int i = start; i < end; i++) {
        const Token& token = tokens[i];
        std::cout << i << ": Type=" << static_cast<int>(token.type) 
                  << ", Value='" << token.value << "'"
                  << ", Line=" << token.line << ", Column=" << token.column;
        if (i == target) {
            std::cout << " <-- STUCK HERE";
        }
        std::cout << std::endl;
    }
    
    return 0;
}