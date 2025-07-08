#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "parser.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

// Type system
enum class GDType {
    VOID,
    INT,
    FLOAT,
    STRING,
    BOOL,
    ARRAY,
    DICTIONARY,
    VECTOR2,
    VECTOR3,
    NODE,
    OBJECT,
    VARIANT,
    CUSTOM,
    LAMBDA,
    UNKNOWN
};

struct TypeInfo {
    GDType base_type;
    std::string custom_name;
    std::vector<TypeInfo> generic_params; // For Array[Type], etc.
    
    TypeInfo(GDType type = GDType::UNKNOWN, const std::string& name = "")
        : base_type(type), custom_name(name) {}
    
    bool operator==(const TypeInfo& other) const {
        return base_type == other.base_type && custom_name == other.custom_name;
    }
    
    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }
    
    std::string toString() const;
    bool isCompatibleWith(const TypeInfo& other) const;
    bool isNumeric() const;
};

// Symbol information
struct Symbol {
    std::string name;
    TypeInfo type;
    bool is_constant;
    bool is_static;
    bool is_initialized;
    int declaration_line;
    
    Symbol() : is_constant(false), is_static(false), is_initialized(false), declaration_line(0) {}
    
    Symbol(const std::string& n, const TypeInfo& t, bool constant = false, 
           bool static_var = false, int line = 0)
        : name(n), type(t), is_constant(constant), is_static(static_var), 
          is_initialized(false), declaration_line(line) {}
};

// Function signature
struct FunctionSignature {
    std::string name;
    std::vector<TypeInfo> parameter_types;
    TypeInfo return_type;
    bool is_static;
    bool is_variadic;  // For functions that accept variable arguments like print
    int declaration_line;
    
    FunctionSignature() : is_static(false), is_variadic(false), declaration_line(0) {}
    
    FunctionSignature(const std::string& n, const std::vector<TypeInfo>& params,
                     const TypeInfo& ret, bool static_func = false, bool variadic = false, int line = 0)
        : name(n), parameter_types(params), return_type(ret), 
          is_static(static_func), is_variadic(variadic), declaration_line(line) {}
};

// Class information
struct ClassInfo {
    std::string name;
    std::string base_class;
    std::unordered_map<std::string, Symbol> members;
    std::unordered_map<std::string, FunctionSignature> methods;
    std::vector<std::string> signals;
    int declaration_line;
    
    ClassInfo() : declaration_line(0) {}
    
    ClassInfo(const std::string& n, const std::string& base = "", int line = 0)
        : name(n), base_class(base), declaration_line(line) {}
};

// Scope management
class Scope {
public:
    std::unordered_map<std::string, Symbol> symbols;
    std::unordered_map<std::string, FunctionSignature> functions;
    Scope* parent;
    
    explicit Scope(Scope* p = nullptr) : parent(p) {}
    
    Symbol* findSymbol(const std::string& name);
    FunctionSignature* findFunction(const std::string& name);
    void defineSymbol(const Symbol& symbol);
    void defineFunction(const FunctionSignature& function);
};

// Semantic analyzer
class SemanticAnalyzer {
private:
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Symbol tables
    std::unique_ptr<Scope> global_scope;
    Scope* current_scope;
    
    // Type information
    std::unordered_map<std::string, ClassInfo> classes;
    std::unordered_map<std::string, TypeInfo> builtin_types;
    
    // Current context
    std::string current_class;
    std::string current_function;
    bool in_loop;
    TypeInfo expected_return_type;
    
    // Helper methods
    void addError(const std::string& message, int line = 0);
    void addWarning(const std::string& message, int line = 0);
    
    void initializeBuiltinTypes();
    TypeInfo getBuiltinType(const std::string& name);
    TypeInfo inferType(Expression* expr);
    TypeInfo getExpressionType(Expression* expr);
    
    bool isAssignmentCompatible(const TypeInfo& target, const TypeInfo& source);
    bool areTypesCompatible(const TypeInfo& left, const TypeInfo& right, TokenType op);
    
    void enterScope();
    void exitScope();
    
    // Analysis methods
    void analyzeProgram(Program* program);
    void analyzeStatement(Statement* stmt);
    void analyzeExpression(Expression* expr);
    void analyzeDeclaration(Declaration* decl);
    
    void analyzeVarDecl(VarDecl* decl);
    void analyzeConstDecl(ConstDecl* decl);
    void analyzeFuncDecl(FuncDecl* decl);
    void analyzeClassDecl(ClassDecl* decl);
    void analyzeSignalDecl(SignalDecl* decl);
    void analyzeEnumDecl(EnumDecl* decl);
    
    void analyzeBlockStmt(BlockStmt* stmt);
    void analyzeIfStmt(IfStmt* stmt);
    void analyzeWhileStmt(WhileStmt* stmt);
    void analyzeForStmt(ForStmt* stmt);
    void analyzeMatchStmt(MatchStmt* stmt);
    void analyzeReturnStmt(ReturnStmt* stmt);
    void analyzeExpressionStmt(ExpressionStmt* stmt);
    
    void analyzeLiteralExpr(LiteralExpr* expr);
    void analyzeIdentifierExpr(IdentifierExpr* expr);
    void analyzeBinaryOpExpr(BinaryOpExpr* expr);
    void analyzeUnaryOpExpr(UnaryOpExpr* expr);
    void analyzeCallExpr(CallExpr* expr);
    void analyzeMemberAccessExpr(MemberAccessExpr* expr);
    void analyzeArrayAccessExpr(ArrayAccessExpr* expr);
    void analyzeArrayLiteralExpr(ArrayLiteralExpr* expr);
    void analyzeDictLiteralExpr(DictLiteralExpr* expr);
    void analyzeLambdaExpr(LambdaExpr* expr);
    void analyzeTernaryExpr(TernaryExpr* expr);
    
    // Type checking utilities
    TypeInfo getUnaryResultType(TokenType op, const TypeInfo& operand);
    TypeInfo getBinaryResultType(const TypeInfo& left, TokenType op, const TypeInfo& right);
    
    // Built-in function checking
    bool isBuiltinFunction(const std::string& name);
    FunctionSignature getBuiltinFunction(const std::string& name);
    
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer() = default;
    
    void analyze(ASTNode* root);
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
    
    const std::vector<std::string>& getErrors() const { return errors; }
    const std::vector<std::string>& getWarnings() const { return warnings; }
    
    // Access to symbol information for code generation
    const std::unordered_map<std::string, ClassInfo>& getClasses() const { return classes; }
    Scope* getGlobalScope() const { return global_scope.get(); }
};

#endif // SEMANTIC_ANALYZER_H