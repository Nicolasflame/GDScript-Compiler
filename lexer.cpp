#include "lexer.h"
#include <iostream>
#include <cctype>

// Initialize keywords map
std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"if", TokenType::IF}, {"elif", TokenType::ELIF}, {"else", TokenType::ELSE},
    {"for", TokenType::FOR}, {"while", TokenType::WHILE}, {"match", TokenType::MATCH},
    {"when", TokenType::WHEN}, {"break", TokenType::BREAK}, {"continue", TokenType::CONTINUE},
    {"pass", TokenType::PASS}, {"return", TokenType::RETURN}, {"class", TokenType::CLASS},
    {"class_name", TokenType::CLASS_NAME}, {"extends", TokenType::EXTENDS},
    {"is", TokenType::IS}, {"in", TokenType::IN}, {"as", TokenType::AS},
    {"self", TokenType::SELF}, {"super", TokenType::SUPER}, {"signal", TokenType::SIGNAL},
    {"func", TokenType::FUNC}, {"static", TokenType::STATIC}, {"const", TokenType::CONST},
    {"enum", TokenType::ENUM}, {"var", TokenType::VAR}, {"breakpoint", TokenType::BREAKPOINT},
    {"preload", TokenType::PRELOAD}, {"await", TokenType::AWAIT}, {"yield", TokenType::YIELD},
    {"assert", TokenType::ASSERT}, {"void", TokenType::VOID}, {"and", TokenType::AND},
    {"or", TokenType::OR}, {"not", TokenType::NOT}, {"lambda", TokenType::LAMBDA},
    {"true", TokenType::BOOLEAN}, {"false", TokenType::BOOLEAN}, {"null", TokenType::NULL_LITERAL}
};

Lexer::Lexer(const std::string& source) 
    : source(source), current(0), line(1), column(1) {
    indent_stack.push_back(0); // Start with no indentation
}

char Lexer::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[current++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

bool Lexer::isAlpha(char c) const {
    return std::isalpha(c) || c == '_';
}

bool Lexer::isDigit(char c) const {
    return std::isdigit(c);
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

void Lexer::addToken(TokenType type, const std::string& value) {
    tokens.emplace_back(type, value, line, column);
}

void Lexer::addError(const std::string& message) {
    errors.push_back("Line " + std::to_string(line) + ", Column " + 
                    std::to_string(column) + ": " + message);
}

void Lexer::scanString(char quote) {
    std::string value;
    
    while (peek() != quote && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 1;
        }
        
        if (peek() == '\\') {
            advance(); // consume backslash
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default:
                    value += escaped;
                    break;
            }
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        addError("Unterminated string");
        return;
    }
    
    advance(); // consume closing quote
    addToken(TokenType::STRING, value);
}

void Lexer::scanNumber() {
    std::string value;
    bool isFloat = false;
    
    while (isDigit(peek())) {
        value += advance();
    }
    
    // Look for decimal point
    if (peek() == '.' && isDigit(peek(1))) {
        isFloat = true;
        value += advance(); // consume '.'
        
        while (isDigit(peek())) {
            value += advance();
        }
    }
    
    // Look for scientific notation
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        value += advance();
        
        if (peek() == '+' || peek() == '-') {
            value += advance();
        }
        
        while (isDigit(peek())) {
            value += advance();
        }
    }
    
    addToken(isFloat ? TokenType::FLOAT : TokenType::INTEGER, value);
}

void Lexer::scanIdentifier() {
    std::string value;
    
    while (isAlphaNumeric(peek())) {
        value += advance();
    }
    
    // Check if it's a keyword
    auto it = keywords.find(value);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    
    addToken(type, value);
}

void Lexer::scanComment() {
    // Single line comment
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

void Lexer::handleIndentation() {
    int indent_level = 0;
    
    // Count leading whitespace
    while (peek() == ' ' || peek() == '\t') {
        if (peek() == ' ') {
            indent_level++;
        } else { // tab
            indent_level += 4; // treat tab as 4 spaces
        }
        advance();
    }
    
    // Skip empty lines and comments
    if (peek() == '\n' || peek() == '#') {
        return;
    }
    
    int current_indent = indent_stack.back();
    
    if (indent_level > current_indent) {
        // Increased indentation
        indent_stack.push_back(indent_level);
        addToken(TokenType::INDENT);
    } else if (indent_level < current_indent) {
        // Decreased indentation
        while (!indent_stack.empty() && indent_stack.back() > indent_level) {
            indent_stack.pop_back();
            addToken(TokenType::DEDENT);
        }
        
        if (indent_stack.empty() || indent_stack.back() != indent_level) {
            addError("Invalid indentation level");
        }
    }
}

void Lexer::scanToken() {
    char c = advance();
    
    switch (c) {
        case ' ':
        case '\r':
        case '\t':
            // Ignore whitespace
            break;
            
        case '\n':
            addToken(TokenType::NEWLINE);
            handleIndentation();
            break;
            
        case '#':
            scanComment();
            break;
            
        case '@':
            // Annotation
            if (isAlpha(peek())) {
                std::string annotation = "@";
                while (isAlphaNumeric(peek())) {
                    annotation += advance();
                }
                addToken(TokenType::ANNOTATION, annotation);
            } else {
                addError("Invalid annotation");
            }
            break;
            
        case '(':
            addToken(TokenType::LEFT_PAREN);
            break;
        case ')':
            addToken(TokenType::RIGHT_PAREN);
            break;
        case '[':
            addToken(TokenType::LEFT_BRACKET);
            break;
        case ']':
            addToken(TokenType::RIGHT_BRACKET);
            break;
        case '{':
            addToken(TokenType::LEFT_BRACE);
            break;
        case '}':
            addToken(TokenType::RIGHT_BRACE);
            break;
        case ',':
            addToken(TokenType::COMMA);
            break;
        case '.':
            addToken(TokenType::DOT);
            break;
        case ':':
            if (peek() == '=') {
                advance();
                addToken(TokenType::TYPE_INFER_ASSIGN);
            } else {
                addToken(TokenType::COLON);
            }
            break;
        case ';':
            addToken(TokenType::SEMICOLON);
            break;
        case '$':
            addToken(TokenType::DOLLAR);
            break;
        case '+':
            if (peek() == '=') {
                advance();
                addToken(TokenType::PLUS_ASSIGN);
            } else {
                addToken(TokenType::PLUS);
            }
            break;
            
        case '-':
            if (peek() == '=') {
                advance();
                addToken(TokenType::MINUS_ASSIGN);
            } else if (peek() == '>') {
                advance();
                addToken(TokenType::ARROW);
            } else {
                addToken(TokenType::MINUS);
            }
            break;
            
        case '*':
            if (peek() == '=') {
                advance();
                addToken(TokenType::MULTIPLY_ASSIGN);
            } else {
                addToken(TokenType::MULTIPLY);
            }
            break;
            
        case '/':
            if (peek() == '=') {
                advance();
                addToken(TokenType::DIVIDE_ASSIGN);
            } else {
                addToken(TokenType::DIVIDE);
            }
            break;
            
        case '%':
            if (peek() == '=') {
                advance();
                addToken(TokenType::MODULO_ASSIGN);
            } else {
                addToken(TokenType::MODULO);
            }
            break;
            
        case '=':
            if (peek() == '=') {
                advance();
                addToken(TokenType::EQUAL);
            } else {
                addToken(TokenType::ASSIGN);
            }
            break;
            
        case '!':
            if (peek() == '=') {
                advance();
                addToken(TokenType::NOT_EQUAL);
            } else {
                addToken(TokenType::LOGICAL_NOT);
            }
            break;
            
        case '<':
            if (peek() == '=') {
                advance();
                addToken(TokenType::LESS_EQUAL);
            } else if (peek() == '<') {
                advance();
                addToken(TokenType::LEFT_SHIFT);
            } else {
                addToken(TokenType::LESS);
            }
            break;
            
        case '>':
            if (peek() == '=') {
                advance();
                addToken(TokenType::GREATER_EQUAL);
            } else if (peek() == '>') {
                advance();
                addToken(TokenType::RIGHT_SHIFT);
            } else {
                addToken(TokenType::GREATER);
            }
            break;
            
        case '&':
            if (peek() == '&') {
                advance();
                addToken(TokenType::LOGICAL_AND);
            } else {
                addToken(TokenType::BITWISE_AND);
            }
            break;
            
        case '|':
            if (peek() == '|') {
                advance();
                addToken(TokenType::LOGICAL_OR);
            } else {
                addToken(TokenType::BITWISE_OR);
            }
            break;
            
        case '^':
            addToken(TokenType::BITWISE_XOR);
            break;
            
        case '~':
            addToken(TokenType::BITWISE_NOT);
            break;
            
        case '"':
        case '\'':
            scanString(c);
            break;
            
        default:
            if (isDigit(c)) {
                current--; // back up
                column--;
                scanNumber();
            } else if (isAlpha(c)) {
                current--; // back up
                column--;
                scanIdentifier();
            } else {
                addError("Unexpected character: " + std::string(1, c));
            }
            break;
    }
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        scanToken();
    }
    
    // Add a final newline if the file doesn't end with one
    if (!tokens.empty() && tokens.back().type != TokenType::NEWLINE) {
        addToken(TokenType::NEWLINE);
    }
    
    // Add final dedents
    while (indent_stack.size() > 1) {
        indent_stack.pop_back();
        addToken(TokenType::DEDENT);
    }
    
    addToken(TokenType::EOF_TOKEN);
    
    // Print errors if any
    for (const auto& error : errors) {
        std::cerr << error << std::endl;
    }
    
    return tokens;
}