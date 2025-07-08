#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Total tokens: " << tokens.size() << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    // Display all tokens
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << i << ": Type=" << static_cast<int>(tokens[i].type) 
                  << ", Value='" << tokens[i].value << "'"
                  << ", Line=" << tokens[i].line 
                  << ", Column=" << tokens[i].column << std::endl;
    }
    
    return 0;
}