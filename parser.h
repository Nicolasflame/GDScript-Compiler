#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class ASTNode;
class Expression;
class Statement;
class Declaration;

// AST Node Types
enum class ASTNodeType {
    // Expressions
    LITERAL,
    IDENTIFIER,
    BINARY_OP,
    UNARY_OP,
    TERNARY,
    CALL,
    MEMBER_ACCESS,
    ARRAY_ACCESS,
    ARRAY_LITERAL,
    DICT_LITERAL,
    LAMBDA,
    
    // Statements
    EXPRESSION_STMT,
    BLOCK,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    MATCH_STMT,
    RETURN_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    PASS_STMT,
    
    // Declarations
    VAR_DECL,
    CONST_DECL,
    FUNC_DECL,
    CLASS_DECL,
    ENUM_DECL,
    SIGNAL_DECL,
    
    // Special
    PROGRAM,
    PARAMETER,
    ARGUMENT
};

// Base AST Node
class ASTNode {
public:
    ASTNodeType type;
    int line;
    int column;
    
    ASTNode(ASTNodeType t, int l = 0, int c = 0) : type(t), line(l), column(c) {}
    virtual ~ASTNode() = default;
};

// Expression nodes
class Expression : public ASTNode {
public:
    Expression(ASTNodeType t, int l = 0, int c = 0) : ASTNode(t, l, c) {}
};

class LiteralExpr : public Expression {
public:
    std::string value;
    TokenType literal_type;
    
    LiteralExpr(const std::string& v, TokenType lt, int l = 0, int c = 0)
        : Expression(ASTNodeType::LITERAL, l, c), value(v), literal_type(lt) {}
};

class IdentifierExpr : public Expression {
public:
    std::string name;
    
    IdentifierExpr(const std::string& n, int l = 0, int c = 0)
        : Expression(ASTNodeType::IDENTIFIER, l, c), name(n) {}
};

class BinaryOpExpr : public Expression {
public:
    std::unique_ptr<Expression> left;
    TokenType operator_type;
    std::unique_ptr<Expression> right;
    
    BinaryOpExpr(std::unique_ptr<Expression> l, TokenType op, std::unique_ptr<Expression> r, int line = 0, int col = 0)
        : Expression(ASTNodeType::BINARY_OP, line, col), left(std::move(l)), operator_type(op), right(std::move(r)) {}
};

class UnaryOpExpr : public Expression {
public:
    TokenType operator_type;
    std::unique_ptr<Expression> operand;
    
    UnaryOpExpr(TokenType op, std::unique_ptr<Expression> expr, int l = 0, int c = 0)
        : Expression(ASTNodeType::UNARY_OP, l, c), operator_type(op), operand(std::move(expr)) {}
};

class TernaryExpr : public Expression {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> true_expr;
    std::unique_ptr<Expression> false_expr;
    
    TernaryExpr(std::unique_ptr<Expression> cond, std::unique_ptr<Expression> true_val, std::unique_ptr<Expression> false_val, int l = 0, int c = 0)
        : Expression(ASTNodeType::TERNARY, l, c), condition(std::move(cond)), true_expr(std::move(true_val)), false_expr(std::move(false_val)) {}
};

class CallExpr : public Expression {
public:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    CallExpr(std::unique_ptr<Expression> c, std::vector<std::unique_ptr<Expression>> args, int l = 0, int col = 0)
        : Expression(ASTNodeType::CALL, l, col), callee(std::move(c)), arguments(std::move(args)) {}
};

class MemberAccessExpr : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string member;
    
    MemberAccessExpr(std::unique_ptr<Expression> obj, const std::string& mem, int l = 0, int c = 0)
        : Expression(ASTNodeType::MEMBER_ACCESS, l, c), object(std::move(obj)), member(mem) {}
};

class ArrayAccessExpr : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    
    ArrayAccessExpr(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx, int l = 0, int c = 0)
        : Expression(ASTNodeType::ARRAY_ACCESS, l, c), array(std::move(arr)), index(std::move(idx)) {}
};

class ArrayLiteralExpr : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    
    ArrayLiteralExpr(std::vector<std::unique_ptr<Expression>> elems, int l = 0, int c = 0)
        : Expression(ASTNodeType::ARRAY_LITERAL, l, c), elements(std::move(elems)) {}
};

class DictLiteralExpr : public Expression {
public:
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;
    
    DictLiteralExpr(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> p, int l = 0, int c = 0)
        : Expression(ASTNodeType::DICT_LITERAL, l, c), pairs(std::move(p)) {}
};

class Parameter {
public:
    std::string name;
    std::string type;
    std::unique_ptr<Expression> default_value;
    
    Parameter(const std::string& n, const std::string& t = "", std::unique_ptr<Expression> def = nullptr)
        : name(n), type(t), default_value(std::move(def)) {}
};

class LambdaExpr : public Expression {
public:
    std::vector<Parameter> parameters;
    std::unique_ptr<Expression> body;
    
    LambdaExpr(std::vector<Parameter> params, std::unique_ptr<Expression> b, int l = 0, int c = 0)
        : Expression(ASTNodeType::LAMBDA, l, c), parameters(std::move(params)), body(std::move(b)) {}
};

// Statement nodes
class Statement : public ASTNode {
public:
    Statement(ASTNodeType t, int l = 0, int c = 0) : ASTNode(t, l, c) {}
};

class ExpressionStmt : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    ExpressionStmt(std::unique_ptr<Expression> expr, int l = 0, int c = 0)
        : Statement(ASTNodeType::EXPRESSION_STMT, l, c), expression(std::move(expr)) {}
};

class BlockStmt : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    BlockStmt(std::vector<std::unique_ptr<Statement>> stmts, int l = 0, int c = 0)
        : Statement(ASTNodeType::BLOCK, l, c), statements(std::move(stmts)) {}
};

class IfStmt : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_branch;
    std::unique_ptr<Statement> else_branch;
    
    IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then_stmt, 
           std::unique_ptr<Statement> else_stmt = nullptr, int l = 0, int c = 0)
        : Statement(ASTNodeType::IF_STMT, l, c), condition(std::move(cond)), 
          then_branch(std::move(then_stmt)), else_branch(std::move(else_stmt)) {}
};

class WhileStmt : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> b, int l = 0, int c = 0)
        : Statement(ASTNodeType::WHILE_STMT, l, c), condition(std::move(cond)), body(std::move(b)) {}
};

class ForStmt : public Statement {
public:
    std::string variable;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    
    ForStmt(const std::string& var, std::unique_ptr<Expression> iter, std::unique_ptr<Statement> b, int l = 0, int c = 0)
        : Statement(ASTNodeType::FOR_STMT, l, c), variable(var), iterable(std::move(iter)), body(std::move(b)) {}
};

class MatchCase {
public:
    std::unique_ptr<Expression> pattern;
    std::unique_ptr<Statement> body;
    
    MatchCase(std::unique_ptr<Expression> pat, std::unique_ptr<Statement> b)
        : pattern(std::move(pat)), body(std::move(b)) {}
};

class MatchStmt : public Statement {
public:
    std::unique_ptr<Expression> expression;
    std::vector<MatchCase> cases;
    
    MatchStmt(std::unique_ptr<Expression> expr, std::vector<MatchCase> c, int l = 0, int col = 0)
        : Statement(ASTNodeType::MATCH_STMT, l, col), expression(std::move(expr)), cases(std::move(c)) {}
};

class ReturnStmt : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    ReturnStmt(std::unique_ptr<Expression> val = nullptr, int l = 0, int c = 0)
        : Statement(ASTNodeType::RETURN_STMT, l, c), value(std::move(val)) {}
};

class BreakStmt : public Statement {
public:
    BreakStmt(int l = 0, int c = 0)
        : Statement(ASTNodeType::BREAK_STMT, l, c) {}
};

class ContinueStmt : public Statement {
public:
    ContinueStmt(int l = 0, int c = 0)
        : Statement(ASTNodeType::CONTINUE_STMT, l, c) {}
};

class PassStmt : public Statement {
public:
    PassStmt(int l = 0, int c = 0)
        : Statement(ASTNodeType::PASS_STMT, l, c) {}
};

// Declaration nodes
class Declaration : public Statement {
public:
    Declaration(ASTNodeType t, int l = 0, int c = 0) : Statement(t, l, c) {}
};

class VarDecl : public Declaration {
public:
    std::string name;
    std::string type;
    std::unique_ptr<Expression> initializer;
    bool is_static;
    std::vector<std::string> annotations;
    
    VarDecl(const std::string& n, const std::string& t = "", std::unique_ptr<Expression> init = nullptr, 
            bool static_var = false, int l = 0, int c = 0)
        : Declaration(ASTNodeType::VAR_DECL, l, c), name(n), type(t), 
          initializer(std::move(init)), is_static(static_var) {}
};

class ConstDecl : public Declaration {
public:
    std::string name;
    std::unique_ptr<Expression> value;
    
    ConstDecl(const std::string& n, std::unique_ptr<Expression> val, int l = 0, int c = 0)
        : Declaration(ASTNodeType::CONST_DECL, l, c), name(n), value(std::move(val)) {}
};

class FuncDecl : public Declaration {
public:
    std::string name;
    std::vector<Parameter> parameters;
    std::string return_type;
    std::unique_ptr<Statement> body;
    bool is_static;
    std::vector<std::string> annotations;
    
    FuncDecl(const std::string& n, std::vector<Parameter> params, const std::string& ret_type,
             std::unique_ptr<Statement> b, bool static_func = false, int l = 0, int c = 0)
        : Declaration(ASTNodeType::FUNC_DECL, l, c), name(n), parameters(std::move(params)),
          return_type(ret_type), body(std::move(b)), is_static(static_func) {}
};

class ClassDecl : public Declaration {
public:
    std::string name;
    std::string base_class;
    std::vector<std::unique_ptr<Declaration>> members;
    std::vector<std::string> annotations;
    
    ClassDecl(const std::string& n, const std::string& base, 
              std::vector<std::unique_ptr<Declaration>> mem, int l = 0, int c = 0)
        : Declaration(ASTNodeType::CLASS_DECL, l, c), name(n), base_class(base), members(std::move(mem)) {}
};

class SignalDecl : public Declaration {
public:
    std::string name;
    std::vector<Parameter> parameters;
    
    SignalDecl(const std::string& n, std::vector<Parameter> params, int l = 0, int c = 0)
        : Declaration(ASTNodeType::SIGNAL_DECL, l, c), name(n), parameters(std::move(params)) {}
};

class EnumValue {
public:
    std::string name;
    std::unique_ptr<Expression> value;
    
    EnumValue(const std::string& n, std::unique_ptr<Expression> val = nullptr)
        : name(n), value(std::move(val)) {}
};

class EnumDecl : public Declaration {
public:
    std::string name;
    std::vector<EnumValue> values;
    
    EnumDecl(const std::string& n, std::vector<EnumValue> vals, int l = 0, int c = 0)
        : Declaration(ASTNodeType::ENUM_DECL, l, c), name(n), values(std::move(vals)) {}
};

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    Program(std::vector<std::unique_ptr<Statement>> stmts)
        : ASTNode(ASTNodeType::PROGRAM), statements(std::move(stmts)) {}
};

// Parser class
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::vector<std::string> errors;
    
    // Helper methods
    Token peek(int offset = 0) const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void addError(const std::string& message);
    void synchronize();
    
    // Parsing methods
    std::unique_ptr<Program> program();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Declaration> declaration();
    std::unique_ptr<Statement> blockStatement();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> matchStatement();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> expressionStatement();
    
    std::unique_ptr<VarDecl> varDeclaration();
    std::unique_ptr<ConstDecl> constDeclaration();
    std::unique_ptr<FuncDecl> funcDeclaration();
    std::unique_ptr<ClassDecl> classDeclaration();
    std::unique_ptr<SignalDecl> signalDeclaration();
    std::unique_ptr<EnumDecl> enumDeclaration();
    
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> ternary();
    std::unique_ptr<Expression> logicalOr();
    std::unique_ptr<Expression> logicalAnd();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> call();
    std::unique_ptr<Expression> primary();
    
    std::vector<Parameter> parameters();
    std::vector<std::unique_ptr<Expression>> arguments();
    
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
};

#endif // PARSER_H