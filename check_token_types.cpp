#include "lexer.h"
#include <iostream>

int main() {
    std::cout << "LEFT_BRACE = " << static_cast<int>(TokenType::LEFT_BRACE) << std::endl;
    std::cout << "RIGHT_BRACE = " << static_cast<int>(TokenType::RIGHT_BRACE) << std::endl;
    std::cout << "COMMA = " << static_cast<int>(TokenType::COMMA) << std::endl;
    std::cout << "NEWLINE = " << static_cast<int>(TokenType::NEWLINE) << std::endl;
    std::cout << "INDENT = " << static_cast<int>(TokenType::INDENT) << std::endl;
    std::cout << "DEDENT = " << static_cast<int>(TokenType::DEDENT) << std::endl;
    return 0;
}