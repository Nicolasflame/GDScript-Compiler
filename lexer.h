#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    NULL_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords
    IF, ELIF, ELSE,
    FOR, WHILE, MATCH, WHEN,
    BREAK, CONTINUE, PASS, RETURN,
    CLASS, CLASS_NAME, EXTENDS,
    IS, IN, AS, SELF, SUPER,
    SIGNAL, FUNC, STATIC,
    CONST, ENUM, VAR,
    BREAKPOINT, PRELOAD, AWAIT,
    YIELD, ASSERT, VOID,
    AND, OR, NOT, LAMBDA,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN,
    MULTIPLY_ASSIGN, DIVIDE_ASSIGN, MODULO_ASSIGN,
    TYPE_INFER_ASSIGN, // :=
    EQUAL, NOT_EQUAL, LESS, LESS_EQUAL,
    GREATER, GREATER_EQUAL,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    BITWISE_AND, BITWISE_OR, BITWISE_XOR,
    BITWISE_NOT, LEFT_SHIFT, RIGHT_SHIFT,
    
    // Delimiters
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACKET, RIGHT_BRACKET,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, COLON, SEMICOLON,
    ARROW, DOLLAR, PERCENT,
    
    // Special
    NEWLINE, INDENT, DEDENT,
    EOF_TOKEN, INVALID,
    
    // Annotations
    ANNOTATION
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    std::string source;
    size_t current;
    int line;
    int column;
    std::vector<Token> tokens;
    std::vector<std::string> errors;
    std::vector<int> indent_stack;
    
    // Keywords map
    static std::unordered_map<std::string, TokenType> keywords;
    
    // Helper methods
    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Tokenization methods
    void scanToken();
    void scanString(char quote);
    void scanNumber();
    void scanIdentifier();
    void scanComment();
    void handleIndentation();
    void addToken(TokenType type, const std::string& value = "");
    void addError(const std::string& message);
    
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
};

#endif // LEXER_H