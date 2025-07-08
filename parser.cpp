#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

Token Parser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[pos];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    Token current_token = peek();
    addError(message + " at line " + std::to_string(current_token.line));
    // Always advance to prevent infinite loops, even on error
    return advance();
}

void Parser::addError(const std::string& message) {
    errors.push_back(message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (tokens[current - 1].type == TokenType::NEWLINE) return;
        
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::FUNC:
            case TokenType::VAR:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    int statement_count = 0;
    int last_token_position = -1;
    int stuck_count = 0;
    const int MAX_STUCK_COUNT = 100;
    
    while (!isAtEnd()) {
        // Skip newlines at top level
        while (match({TokenType::NEWLINE})) {
            // Just skip newlines
        }
        
        if (isAtEnd()) {
            break;
        }
        
        // Check for infinite loop - if we're stuck at the same token position
        if (current == last_token_position) {
            stuck_count++;
            if (stuck_count >= MAX_STUCK_COUNT) {
                std::cerr << "Parser stuck in infinite loop at token " << current << " (type: " << static_cast<int>(tokens[current].type) << ", value: '" << tokens[current].value << "')" << std::endl;
                // Force advance to break the loop
                advance();
                stuck_count = 0;
                continue;
            }
        } else {
            stuck_count = 0;
            last_token_position = current;
        }
        
        // Skip empty tokens
        if (tokens[current].value.empty()) {
            advance();
            continue;
        }
        
        try {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
                statement_count++;
            }
        } catch (const std::exception& e) {
            addError(e.what());
            synchronize();
        }
    }
    
    // Print errors
    for (const auto& error : errors) {
        std::cerr << "Parse Error: " << error << std::endl;
    }
    
    return std::make_unique<Program>(std::move(statements));
}

std::unique_ptr<Statement> Parser::statement() {
    // Handle annotations
    std::vector<std::string> annotations;
    while (check(TokenType::ANNOTATION)) {
        annotations.push_back(advance().value);
    }
    
    if (match({TokenType::CLASS_NAME})) {
        Token name_token = consume(TokenType::IDENTIFIER, "Expected class name after 'class_name'");
        consume(TokenType::NEWLINE, "Expected newline after class_name declaration");
        
        // For class_name, we create a simple class declaration without collecting members
        // The members will be parsed separately at the top level
        std::vector<std::unique_ptr<Declaration>> members;
        return std::make_unique<ClassDecl>(name_token.value, "", std::move(members));
    }
    
    if (match({TokenType::EXTENDS})) {
        Token base_token = consume(TokenType::IDENTIFIER, "Expected base class name after 'extends'");
        consume(TokenType::NEWLINE, "Expected newline after extends declaration");
        
        // For top-level extends, we create a simple class declaration with just the base class
        // The members will be parsed separately at the top level
        std::vector<std::unique_ptr<Declaration>> members;
        return std::make_unique<ClassDecl>("", base_token.value, std::move(members));
    }
    
    if (match({TokenType::CLASS})) {
        auto decl = classDeclaration();
        decl->annotations = annotations;
        return std::move(decl);
    }
    // Handle static declarations
    if (match({TokenType::STATIC})) {
        if (match({TokenType::FUNC})) {
            auto decl = funcDeclaration();
            decl->annotations = annotations;
            decl->is_static = true;
            return std::move(decl);
        } else if (match({TokenType::VAR})) {
            auto decl = varDeclaration();
            decl->annotations = annotations;
            decl->is_static = true;
            return std::move(decl);
        } else {
            addError("Expected 'func' or 'var' after 'static'");
            return nullptr;
        }
    }
    
    if (match({TokenType::FUNC})) {
        // This should always be a function declaration at statement level
        // Lambda expressions are only parsed within expressions, not as standalone statements
        auto decl = funcDeclaration();
        decl->annotations = annotations;
        return std::move(decl);
    }
    if (match({TokenType::VAR})) {
        auto decl = varDeclaration();
        decl->annotations = annotations;
        return std::move(decl);
    }
    if (match({TokenType::CONST})) return constDeclaration();
    if (match({TokenType::ENUM})) return enumDeclaration();
    if (match({TokenType::SIGNAL})) return signalDeclaration();
    
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::FOR})) return forStatement();
    if (match({TokenType::MATCH})) return matchStatement();
    if (match({TokenType::RETURN})) return returnStatement();
    
    if (match({TokenType::BREAK})) {
        consume(TokenType::NEWLINE, "Expected newline after 'break'");
        return std::make_unique<Statement>(ASTNodeType::BREAK_STMT);
    }
    
    if (match({TokenType::CONTINUE})) {
        consume(TokenType::NEWLINE, "Expected newline after 'continue'");
        return std::make_unique<Statement>(ASTNodeType::CONTINUE_STMT);
    }
    
    if (match({TokenType::PASS})) {
        consume(TokenType::NEWLINE, "Expected newline after 'pass'");
        return std::make_unique<Statement>(ASTNodeType::PASS_STMT);
    }
    
    // Check for type inference assignment (identifier := expression)
     if (check(TokenType::IDENTIFIER)) {
         int saved_current = current;
         advance(); // consume identifier
         if (match({TokenType::TYPE_INFER_ASSIGN})) {
             // Reset and parse as type inference variable declaration
             current = saved_current;
             Token name_token = consume(TokenType::IDENTIFIER, "Expected variable name");
             consume(TokenType::TYPE_INFER_ASSIGN, "Expected ':=' for type inference");
             auto initializer = expression();
             consume(TokenType::NEWLINE, "Expected newline after type inference assignment");
             return std::make_unique<VarDecl>(name_token.value, "", std::move(initializer));
         } else {
             // Reset and parse as expression statement
             current = saved_current;
         }
     }
     
     return expressionStatement();
}

std::unique_ptr<Statement> Parser::blockStatement() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    consume(TokenType::INDENT, "Expected indentation");
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        // Skip newlines
        while (match({TokenType::NEWLINE})) {
            // Just skip newlines
        }
        
        // Break if we've reached the end of the block
        if (check(TokenType::DEDENT) || isAtEnd()) {
            break;
        }
        
        // Skip empty tokens
        if (tokens[current].value.empty()) {
            advance();
            continue;
        }
        
        auto stmt = statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::DEDENT, "Expected dedentation");
    
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Statement> Parser::ifStatement() {
    auto condition = expression();
    consume(TokenType::COLON, "Expected ':' after if condition");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    auto then_branch = blockStatement();
    std::unique_ptr<Statement> else_branch = nullptr;
    
    if (match({TokenType::ELIF})) {
        else_branch = ifStatement(); // Recursive for elif chain
    } else if (match({TokenType::ELSE})) {
        consume(TokenType::COLON, "Expected ':' after else");
        consume(TokenType::NEWLINE, "Expected newline after ':'");
        else_branch = blockStatement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Statement> Parser::whileStatement() {
    auto condition = expression();
    consume(TokenType::COLON, "Expected ':' after while condition");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    auto body = blockStatement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::forStatement() {
    Token var_token = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::IN, "Expected 'in' after for variable");
    auto iterable = expression();
    consume(TokenType::COLON, "Expected ':' after for expression");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    auto body = blockStatement();
    
    return std::make_unique<ForStmt>(var_token.value, std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::matchStatement() {
    auto expr = expression();
    consume(TokenType::COLON, "Expected ':' after match expression");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    consume(TokenType::INDENT, "Expected indentation");
    
    std::vector<MatchCase> cases;
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (match({TokenType::NEWLINE})) {
            continue;
        }
        
        // Parse pattern
        auto pattern = expression();
        consume(TokenType::COLON, "Expected ':' after match pattern");
        consume(TokenType::NEWLINE, "Expected newline after ':'");
        
        // Parse body
        auto body = blockStatement();
        
        cases.emplace_back(std::move(pattern), std::move(body));
    }
    
    consume(TokenType::DEDENT, "Expected dedentation");
    
    return std::make_unique<MatchStmt>(std::move(expr), std::move(cases));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT)) {
        value = expression();
    }
    
    // Skip any newlines that might have been consumed during expression parsing
    while (match({TokenType::NEWLINE})) {
        // Just skip newlines
    }
    
    // If we don't have a newline or dedent now, we need a newline
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !isAtEnd()) {
        consume(TokenType::NEWLINE, "Expected newline after return statement");
    }
    
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();
    if (!expr) {
        return nullptr;
    }
    
    // Consume newline if present, but don't require it if we're at end of file or about to dedent
    if (check(TokenType::NEWLINE)) {
        advance(); // consume the newline
    } else if (!check(TokenType::DEDENT) && !isAtEnd()) {
        // Only require newline if we're not at dedent or end of file
        consume(TokenType::NEWLINE, "Expected newline after expression");
    }
    
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<VarDecl> Parser::varDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected variable name");
    
    std::string type_hint;
    if (match({TokenType::COLON})) {
        Token type_token = consume(TokenType::IDENTIFIER, "Expected type name");
        type_hint = type_token.value;
        
        // Handle generic types like Array[String]
        if (match({TokenType::LEFT_BRACKET})) {
            type_hint += "[";
            Token generic_type = consume(TokenType::IDENTIFIER, "Expected generic type name");
            type_hint += generic_type.value;
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after generic type");
            type_hint += "]";
        }
    }
    
    std::unique_ptr<Expression> initializer = nullptr;
    if (match({TokenType::ASSIGN, TokenType::TYPE_INFER_ASSIGN})) {
        initializer = expression();
    }
    
    // Consume newline if present, but don't require it if we're at end of file or about to dedent
    if (check(TokenType::NEWLINE)) {
        advance(); // consume the newline
    } else if (!check(TokenType::DEDENT) && !isAtEnd()) {
        // Only require newline if we're not at dedent or end of file
        consume(TokenType::NEWLINE, "Expected newline after variable declaration");
    }
    
    return std::make_unique<VarDecl>(name_token.value, type_hint, std::move(initializer));
}

std::unique_ptr<ConstDecl> Parser::constDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected constant name");
    consume(TokenType::ASSIGN, "Expected '=' after constant name");
    auto value = expression();
    consume(TokenType::NEWLINE, "Expected newline after constant declaration");
    
    return std::make_unique<ConstDecl>(name_token.value, std::move(value));
}

std::unique_ptr<FuncDecl> Parser::funcDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    auto params = parameters();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    std::string return_type;
    if (match({TokenType::ARROW})) {
        if (check(TokenType::VOID)) {
            Token type_token = advance();
            return_type = type_token.value;
        } else {
            Token type_token = consume(TokenType::IDENTIFIER, "Expected return type");
            return_type = type_token.value;
        }
    }
    
    consume(TokenType::COLON, "Expected ':' after function signature");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    auto body = blockStatement();
    
    return std::make_unique<FuncDecl>(name_token.value, std::move(params), return_type, std::move(body));
}

std::unique_ptr<ClassDecl> Parser::classDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected class name");
    
    std::string base_class;
    if (match({TokenType::EXTENDS})) {
        Token base_token = consume(TokenType::IDENTIFIER, "Expected base class name");
        base_class = base_token.value;
    }
    
    consume(TokenType::COLON, "Expected ':' after class declaration");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    consume(TokenType::INDENT, "Expected indentation");
    
    std::vector<std::unique_ptr<Declaration>> members;
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (match({TokenType::NEWLINE})) {
            continue;
        }
        
        auto member = statement();
        if (auto decl = dynamic_cast<Declaration*>(member.get())) {
            member.release();
            members.push_back(std::unique_ptr<Declaration>(decl));
        } else if (member) {
            // If it's not a declaration but is a valid statement, we still need to handle it
            // For now, we'll just skip non-declaration statements in class bodies
            addError("Only declarations are allowed in class bodies");
        }
    }
    
    consume(TokenType::DEDENT, "Expected dedentation");
    
    return std::make_unique<ClassDecl>(name_token.value, base_class, std::move(members));
}

std::unique_ptr<SignalDecl> Parser::signalDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected signal name");
    
    std::vector<Parameter> params;
    if (match({TokenType::LEFT_PAREN})) {
        params = parameters();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after signal parameters");
    }
    
    consume(TokenType::NEWLINE, "Expected newline after signal declaration");
    
    return std::make_unique<SignalDecl>(name_token.value, std::move(params));
}

std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = ternary();
    
    if (!expr) {
        return nullptr;
    }
    
    if (match({TokenType::ASSIGN, TokenType::TYPE_INFER_ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN,
               TokenType::MULTIPLY_ASSIGN, TokenType::DIVIDE_ASSIGN, TokenType::MODULO_ASSIGN})) {
        Token operator_token = tokens[current - 1];
        auto value = assignment();
        if (!value) {
            return nullptr;
        }
        return std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(value));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::ternary() {
    auto expr = logicalOr();
    
    if (match({TokenType::IF})) {
        auto condition = logicalOr();
        consume(TokenType::ELSE, "Expected 'else' in ternary expression");
        auto false_expr = ternary();
        // GDScript syntax: true_expr if condition else false_expr
        return std::make_unique<TernaryExpr>(std::move(condition), std::move(expr), std::move(false_expr));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    if (!expr) {
        return nullptr;
    }
    
    while (match({TokenType::OR, TokenType::LOGICAL_OR})) {
        Token operator_token = tokens[current - 1];
        auto right = logicalAnd();
        if (!right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();
    
    while (match({TokenType::AND, TokenType::LOGICAL_AND})) {
        Token operator_token = tokens[current - 1];
        auto right = equality();
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        Token operator_token = tokens[current - 1];
        auto right = comparison();
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL, TokenType::IN})) {
        Token operator_token = tokens[current - 1];
        auto right = term();
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    
    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token operator_token = tokens[current - 1];
        auto right = factor();
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = unary();
    
    if (!expr) {
        return nullptr;
    }
    
    while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
        Token operator_token = tokens[current - 1];
        auto right = unary();
        if (!right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryOpExpr>(std::move(expr), operator_token.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match({TokenType::NOT, TokenType::LOGICAL_NOT, TokenType::MINUS, TokenType::PLUS})) {
        Token operator_token = tokens[current - 1];
        auto right = unary();
        if (!right) {
            return nullptr;
        }
        return std::make_unique<UnaryOpExpr>(operator_token.type, std::move(right));
    }
    
    return call();
}

std::unique_ptr<Expression> Parser::call() {
    auto expr = primary();
    
    // If primary() failed, return nullptr to prevent infinite loops
    if (!expr) {
        return nullptr;
    }
    
    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            auto args = arguments();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (match({TokenType::DOT})) {
            Token name = consume(TokenType::IDENTIFIER, "Expected property name after '.'");
            expr = std::make_unique<MemberAccessExpr>(std::move(expr), name.value);
        } else if (match({TokenType::LEFT_BRACKET})) {
            auto index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (match({TokenType::BOOLEAN, TokenType::NULL_LITERAL})) {
        Token token = tokens[current - 1];
        return std::make_unique<LiteralExpr>(token.value, token.type);
    }
    
    if (match({TokenType::INTEGER, TokenType::FLOAT, TokenType::STRING})) {
        Token token = tokens[current - 1];
        return std::make_unique<LiteralExpr>(token.value, token.type);
    }
    
    if (match({TokenType::IDENTIFIER})) {
        Token token = tokens[current - 1];
        return std::make_unique<IdentifierExpr>(token.value);
    }
    
    if (match({TokenType::LEFT_PAREN})) {
        auto expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match({TokenType::LEFT_BRACKET})) {
        std::vector<std::unique_ptr<Expression>> elements;
        
        // Skip any initial newlines and indentation
        while (match({TokenType::NEWLINE, TokenType::INDENT})) {
            // Just skip newlines and indentation
        }
        
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                // Skip newlines and indentation between array elements
                while (match({TokenType::NEWLINE, TokenType::INDENT})) {
                    // Just skip newlines and indentation
                }
                
                // Break if we've reached the end of the array
                if (check(TokenType::RIGHT_BRACKET) || check(TokenType::DEDENT) || isAtEnd()) {
                    break;
                }
                
                auto element = expression();
                if (!element) {
                    break;
                }
                elements.push_back(std::move(element));
                
                // Skip newlines after the element
                while (match({TokenType::NEWLINE})) {
                    // Just skip newlines
                }
            } while (match({TokenType::COMMA}));
        }
        
        // Skip any trailing newlines and dedentation before the closing bracket
        while (match({TokenType::NEWLINE, TokenType::DEDENT})) {
            // Just skip newlines and dedentation
        }
        
        consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements");
        return std::make_unique<ArrayLiteralExpr>(std::move(elements));
    }
    
    if (match({TokenType::LEFT_BRACE})) {
        std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;
        
        // Skip any initial newlines and indentation
        while (match({TokenType::NEWLINE, TokenType::INDENT})) {
            // Just skip newlines and indentation
        }
        
        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                // Skip newlines and indentation between dictionary elements
                while (match({TokenType::NEWLINE, TokenType::INDENT})) {
                    // Just skip newlines and indentation
                }
                
                // Break if we've reached the end of the dictionary
                if (check(TokenType::RIGHT_BRACE) || check(TokenType::DEDENT) || isAtEnd()) {
                    break;
                }
                
                auto key = expression();
                if (!key) {
                    break;
                }
                consume(TokenType::COLON, "Expected ':' after dictionary key");
                auto value = expression();
                if (!value) {
                    break;
                }
                pairs.emplace_back(std::move(key), std::move(value));
                
                // Skip newlines after the value
                while (match({TokenType::NEWLINE})) {
                    // Just skip newlines
                }
            } while (match({TokenType::COMMA}));
        }
        
        // Skip any trailing newlines and dedentation before the closing brace
        while (match({TokenType::NEWLINE, TokenType::DEDENT})) {
            // Just skip newlines and dedentation
        }
        
        consume(TokenType::RIGHT_BRACE, "Expected '}' after dictionary elements");
        return std::make_unique<DictLiteralExpr>(std::move(pairs));
    }
    
    if (match({TokenType::FUNC})) {
        std::vector<Parameter> params;
        
        consume(TokenType::LEFT_PAREN, "Expected '(' after 'func'");
        params = parameters();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after lambda parameters");
        
        consume(TokenType::COLON, "Expected ':' after lambda parameters");
        auto body = expression();
        
        return std::make_unique<LambdaExpr>(std::move(params), std::move(body));
    }
    
    addError("Expected expression");
    return nullptr;
}

std::vector<Parameter> Parser::parameters() {
    std::vector<Parameter> params;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            
            std::string type;
            if (match({TokenType::COLON})) {
                Token type_token = consume(TokenType::IDENTIFIER, "Expected parameter type");
                type = type_token.value;
            }
            
            std::unique_ptr<Expression> default_value = nullptr;
            if (match({TokenType::ASSIGN})) {
                default_value = expression();
            }
            
            params.emplace_back(name.value, type, std::move(default_value));
        } while (match({TokenType::COMMA}));
    }
    
    return params;
}

std::vector<std::unique_ptr<Expression>> Parser::arguments() {
    std::vector<std::unique_ptr<Expression>> args;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    
    return args;
}

std::unique_ptr<EnumDecl> Parser::enumDeclaration() {
    Token name_token = consume(TokenType::IDENTIFIER, "Expected enum name");
    consume(TokenType::LEFT_BRACE, "Expected '{' after enum name");
    
    std::vector<EnumValue> values;
    
    // Skip any initial newlines
    while (match({TokenType::NEWLINE})) {
        // Just skip newlines
    }
    
    // If we immediately see a right brace, it's an empty enum
    if (!check(TokenType::RIGHT_BRACE)) {
        do {
            // Skip newlines between enum values
            while (match({TokenType::NEWLINE})) {
                // Just skip newlines
            }
            
            // Skip indentation/dedentation if present
            while (match({TokenType::INDENT, TokenType::DEDENT})) {
                // Just skip indentation/dedentation
            }
            
            // Break if we've reached the end of the enum
            if (check(TokenType::RIGHT_BRACE) || isAtEnd()) {
                break;
            }
            
            // Skip empty tokens
            if (tokens[current].value.empty()) {
                advance();
                continue;
            }
            
            // Make sure we have an identifier
            if (!check(TokenType::IDENTIFIER)) {
                addError("Expected enum value name, got " + tokens[current].value);
                // Try to recover by skipping this token
                advance();
                continue;
            }
            
            Token value_name = consume(TokenType::IDENTIFIER, "Expected enum value name");
            std::unique_ptr<Expression> value_expr = nullptr;
            
            if (match({TokenType::ASSIGN})) {
                value_expr = expression();
            }
            
            values.emplace_back(value_name.value, std::move(value_expr));
            
            // If no comma, we're done with the enum values
            if (!match({TokenType::COMMA})) {
                break;
            }
        } while (!check(TokenType::RIGHT_BRACE) && !isAtEnd());
    }
    
    // Skip any trailing newlines and dedents before the closing brace
    while (match({TokenType::NEWLINE, TokenType::DEDENT})) {
        // Just skip newlines and dedents
    }
    
    // Try to recover if we don't find the closing brace
    if (!check(TokenType::RIGHT_BRACE)) {
        addError("Expected '}' after enum values");
        // Try to synchronize to a newline
        while (!check(TokenType::NEWLINE) && !isAtEnd()) {
            advance();
        }
    } else {
        consume(TokenType::RIGHT_BRACE, "Expected '}' after enum values");
    }
    
    // Try to recover if we don't find a newline
    if (!check(TokenType::NEWLINE)) {
        addError("Expected newline after enum declaration");
    } else {
        consume(TokenType::NEWLINE, "Expected newline after enum declaration");
    }
    
    return std::make_unique<EnumDecl>(name_token.value, std::move(values));
}