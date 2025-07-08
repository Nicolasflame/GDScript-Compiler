#include "semantic_analyzer.h"
#include <iostream>
#include <sstream>

// TypeInfo implementation
std::string TypeInfo::toString() const {
    std::string result;
    switch (base_type) {
        case GDType::VOID: result = "void"; break;
        case GDType::INT: result = "int"; break;
        case GDType::FLOAT: result = "float"; break;
        case GDType::STRING: result = "String"; break;
        case GDType::BOOL: result = "bool"; break;
        case GDType::ARRAY: result = "Array"; break;
        case GDType::DICTIONARY: result = "Dictionary"; break;
        case GDType::VECTOR2: result = "Vector2"; break;
        case GDType::VECTOR3: result = "Vector3"; break;
        case GDType::NODE: result = "Node"; break;
        case GDType::OBJECT: result = "Object"; break;
        case GDType::VARIANT: result = "Variant"; break;
        case GDType::CUSTOM: result = custom_name; break;
        case GDType::LAMBDA: result = "lambda"; break;
        case GDType::UNKNOWN: result = "unknown"; break;
        default: result = "unknown"; break;
    }
    
    // Add generic parameters if any
    if (!generic_params.empty()) {
        result += "[";
        for (size_t i = 0; i < generic_params.size(); ++i) {
            if (i > 0) result += ", ";
            result += generic_params[i].toString();
        }
        result += "]";
    }
    
    return result;
}

bool TypeInfo::isCompatibleWith(const TypeInfo& other) const {
    if (*this == other) return true;
    
    // Variant can accept any type
    if (other.base_type == GDType::VARIANT || base_type == GDType::VARIANT) return true;
    
    // Numeric conversions
    if (isNumeric() && other.isNumeric()) return true;
    
    // String can accept any type (for concatenation/conversion)
    if (other.base_type == GDType::STRING) return true;
    
    // Object hierarchy (simplified)
    if (base_type == GDType::NODE && other.base_type == GDType::OBJECT) return true;
    if (base_type == GDType::OBJECT && other.base_type == GDType::NODE) return true;
    
    return false;
}

bool TypeInfo::isNumeric() const {
    return base_type == GDType::INT || base_type == GDType::FLOAT;
}

// Scope implementation
Symbol* Scope::findSymbol(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &it->second;
    }
    
    if (parent) {
        return parent->findSymbol(name);
    }
    
    return nullptr;
}

FunctionSignature* Scope::findFunction(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    
    if (parent) {
        return parent->findFunction(name);
    }
    
    return nullptr;
}

void Scope::defineSymbol(const Symbol& symbol) {
    symbols[symbol.name] = symbol;
}

void Scope::defineFunction(const FunctionSignature& function) {
    functions[function.name] = function;
}

// SemanticAnalyzer implementation
SemanticAnalyzer::SemanticAnalyzer() 
    : global_scope(std::make_unique<Scope>()), current_scope(global_scope.get()),
      in_loop(false), expected_return_type(GDType::VOID) {
    initializeBuiltinTypes();
}

void SemanticAnalyzer::addError(const std::string& message, int line) {
    std::string error_msg = "Semantic Error";
    if (line > 0) {
        error_msg += " at line " + std::to_string(line);
    }
    error_msg += ": " + message;
    errors.push_back(error_msg);
}

void SemanticAnalyzer::addWarning(const std::string& message, int line) {
    std::string warning_msg = "Warning";
    if (line > 0) {
        warning_msg += " at line " + std::to_string(line);
    }
    warning_msg += ": " + message;
    warnings.push_back(warning_msg);
}

void SemanticAnalyzer::initializeBuiltinTypes() {
    builtin_types["int"] = TypeInfo(GDType::INT);
    builtin_types["float"] = TypeInfo(GDType::FLOAT);
    builtin_types["String"] = TypeInfo(GDType::STRING);
    builtin_types["bool"] = TypeInfo(GDType::BOOL);
    builtin_types["Array"] = TypeInfo(GDType::ARRAY);
    builtin_types["Dictionary"] = TypeInfo(GDType::DICTIONARY);
    builtin_types["Vector2"] = TypeInfo(GDType::VECTOR2);
    builtin_types["Vector3"] = TypeInfo(GDType::VECTOR3);
    builtin_types["Node"] = TypeInfo(GDType::NODE);
    builtin_types["Object"] = TypeInfo(GDType::OBJECT);
    builtin_types["Variant"] = TypeInfo(GDType::VARIANT);
    builtin_types["void"] = TypeInfo(GDType::VOID);
    
    // Add built-in functions
    // print() - variadic function that accepts any number of arguments
    std::vector<TypeInfo> print_params = {}; // Empty params for variadic
    global_scope->defineFunction(FunctionSignature("print", print_params, TypeInfo(GDType::VOID), false, true));
    
    std::vector<TypeInfo> range_params = {TypeInfo(GDType::INT)};
    global_scope->defineFunction(FunctionSignature("range", range_params, TypeInfo(GDType::ARRAY)));
    
    std::vector<TypeInfo> len_params = {TypeInfo(GDType::VARIANT)};
    global_scope->defineFunction(FunctionSignature("len", len_params, TypeInfo(GDType::INT)));
    
    // str() - converts any value to string
    std::vector<TypeInfo> str_params = {TypeInfo(GDType::VARIANT)};
    global_scope->defineFunction(FunctionSignature("str", str_params, TypeInfo(GDType::STRING)));
}

TypeInfo SemanticAnalyzer::getBuiltinType(const std::string& name) {
    // Check for generic types like Array[String]
    size_t bracket_pos = name.find('[');
    if (bracket_pos != std::string::npos) {
        std::string base_type = name.substr(0, bracket_pos);
        size_t end_bracket = name.find(']', bracket_pos);
        if (end_bracket != std::string::npos) {
            std::string generic_param = name.substr(bracket_pos + 1, end_bracket - bracket_pos - 1);
            
            // Check if base type exists
            auto base_it = builtin_types.find(base_type);
            if (base_it != builtin_types.end()) {
                TypeInfo result = base_it->second;
                // Add generic parameter
                TypeInfo param_type = getBuiltinType(generic_param);
                if (param_type.base_type != GDType::UNKNOWN) {
                    result.generic_params.push_back(param_type);
                }
                return result;
            }
        }
    }
    
    auto it = builtin_types.find(name);
    if (it != builtin_types.end()) {
        return it->second;
    }
    
    // Check if it's a custom class
    auto class_it = classes.find(name);
    if (class_it != classes.end()) {
        return TypeInfo(GDType::CUSTOM, name);
    }
    
    return TypeInfo(GDType::UNKNOWN);
}

void SemanticAnalyzer::analyzeMatchStmt(MatchStmt* stmt) {
    // Analyze the match expression
    analyzeExpression(stmt->expression.get());
    TypeInfo match_type = getExpressionType(stmt->expression.get());
    
    // Analyze each match case
    for (auto& match_case : stmt->cases) {
        // Analyze the pattern (which is an expression)
        analyzeExpression(match_case.pattern.get());
        TypeInfo pattern_type = getExpressionType(match_case.pattern.get());
        
        // Check if pattern type is compatible with match expression type
        if (!pattern_type.isCompatibleWith(match_type) && 
            pattern_type.base_type != GDType::VARIANT && 
            match_type.base_type != GDType::VARIANT) {
            addWarning("Pattern type " + pattern_type.toString() + 
                      " may not match expression type " + match_type.toString(), 
                      match_case.pattern->line);
        }
        
        // Analyze the case body
        analyzeStatement(match_case.body.get());
    }
}

void SemanticAnalyzer::analyzeLambdaExpr(LambdaExpr* expr) {
    // Enter a new scope for the lambda
    enterScope();
    
    // Add lambda parameters to the scope
    for (const auto& param : expr->parameters) {
        Symbol symbol;
        symbol.name = param.name;
        symbol.type = TypeInfo(GDType::VARIANT); // Default to Variant for lambda params
        symbol.is_initialized = true;
        current_scope->defineSymbol(symbol);
    }
    
    // Analyze the lambda body
    analyzeExpression(expr->body.get());
    
    // Exit lambda scope
    exitScope();
}

void SemanticAnalyzer::analyzeTernaryExpr(TernaryExpr* expr) {
    // Analyze all three parts of the ternary expression
    analyzeExpression(expr->condition.get());
    analyzeExpression(expr->true_expr.get());
    analyzeExpression(expr->false_expr.get());
    
    // Check that condition is boolean-compatible
    TypeInfo condition_type = getExpressionType(expr->condition.get());
    if (condition_type.base_type != GDType::BOOL && 
        condition_type.base_type != GDType::VARIANT &&
        condition_type.base_type != GDType::UNKNOWN) {
        addWarning("Ternary condition should be boolean, got " + condition_type.toString(), expr->line);
    }
    
    // Check type compatibility between true and false expressions
    TypeInfo true_type = getExpressionType(expr->true_expr.get());
    TypeInfo false_type = getExpressionType(expr->false_expr.get());
    
    if (true_type != false_type && 
        true_type.base_type != GDType::VARIANT && 
        false_type.base_type != GDType::VARIANT &&
        true_type.base_type != GDType::UNKNOWN &&
        false_type.base_type != GDType::UNKNOWN) {
        addWarning("Ternary branches have different types: " + 
                  true_type.toString() + " and " + false_type.toString(), expr->line);
    }
}

void SemanticAnalyzer::enterScope() {
    auto new_scope = std::make_unique<Scope>(current_scope);
    current_scope = new_scope.release();
}

void SemanticAnalyzer::exitScope() {
    if (current_scope && current_scope->parent) {
        Scope* old_scope = current_scope;
        current_scope = current_scope->parent;
        delete old_scope;
    }
}

void SemanticAnalyzer::analyze(ASTNode* root) {
    if (!root) {
        addError("No AST to analyze");
        return;
    }
    
    if (root->type == ASTNodeType::PROGRAM) {
        analyzeProgram(static_cast<Program*>(root));
    } else {
        addError("Root node is not a program");
    }
    
    // Print errors and warnings
    for (const auto& error : errors) {
        std::cerr << error << std::endl;
    }
    
    for (const auto& warning : warnings) {
        std::cerr << warning << std::endl;
    }
}

void SemanticAnalyzer::analyzeProgram(Program* program) {
    for (auto& stmt : program->statements) {
        analyzeStatement(stmt.get());
    }
}

void SemanticAnalyzer::analyzeStatement(Statement* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case ASTNodeType::VAR_DECL:
            analyzeVarDecl(static_cast<VarDecl*>(stmt));
            break;
        case ASTNodeType::CONST_DECL:
            analyzeConstDecl(static_cast<ConstDecl*>(stmt));
            break;
        case ASTNodeType::FUNC_DECL:
            analyzeFuncDecl(static_cast<FuncDecl*>(stmt));
            break;
        case ASTNodeType::CLASS_DECL:
            analyzeClassDecl(static_cast<ClassDecl*>(stmt));
            break;
        case ASTNodeType::SIGNAL_DECL:
        analyzeSignalDecl(static_cast<SignalDecl*>(stmt));
        break;
    case ASTNodeType::ENUM_DECL:
        analyzeEnumDecl(static_cast<EnumDecl*>(stmt));
        break;
        case ASTNodeType::BLOCK:
            analyzeBlockStmt(static_cast<BlockStmt*>(stmt));
            break;
        case ASTNodeType::IF_STMT:
            analyzeIfStmt(static_cast<IfStmt*>(stmt));
            break;
        case ASTNodeType::WHILE_STMT:
            analyzeWhileStmt(static_cast<WhileStmt*>(stmt));
            break;
        case ASTNodeType::FOR_STMT:
            analyzeForStmt(static_cast<ForStmt*>(stmt));
            break;
        case ASTNodeType::MATCH_STMT:
            analyzeMatchStmt(static_cast<MatchStmt*>(stmt));
            break;
        case ASTNodeType::RETURN_STMT:
            analyzeReturnStmt(static_cast<ReturnStmt*>(stmt));
            break;
        case ASTNodeType::EXPRESSION_STMT:
            analyzeExpressionStmt(static_cast<ExpressionStmt*>(stmt));
            break;
        case ASTNodeType::BREAK_STMT:
        case ASTNodeType::CONTINUE_STMT:
            if (!in_loop) {
                addError("Break/continue statement outside of loop", stmt->line);
            }
            break;
        case ASTNodeType::PASS_STMT:
            // Pass statements are always valid
            break;
        default:
            addError("Unknown statement type", stmt->line);
            break;
    }
}

void SemanticAnalyzer::analyzeVarDecl(VarDecl* decl) {
    TypeInfo declared_type = TypeInfo(GDType::VARIANT);
    
    if (!decl->type.empty()) {
        declared_type = getBuiltinType(decl->type);
        if (declared_type.base_type == GDType::UNKNOWN) {
            addError("Unknown type '" + decl->type + "'", decl->line);
        }
    }
    
    TypeInfo inferred_type = declared_type;
    
    if (decl->initializer) {
        analyzeExpression(decl->initializer.get());
        inferred_type = getExpressionType(decl->initializer.get());
        
        if (!declared_type.isCompatibleWith(inferred_type) && declared_type.base_type != GDType::VARIANT) {
            addError("Type mismatch: cannot assign " + inferred_type.toString() + 
                    " to " + declared_type.toString(), decl->line);
        }
    }
    
    // Check for redefinition
    if (current_scope->symbols.find(decl->name) != current_scope->symbols.end()) {
        addError("Variable '" + decl->name + "' already defined", decl->line);
    }
    
    Symbol symbol(decl->name, declared_type.base_type != GDType::VARIANT ? declared_type : inferred_type, 
                  false, decl->is_static, decl->line);
    symbol.is_initialized = (decl->initializer != nullptr);
    current_scope->defineSymbol(symbol);
}

void SemanticAnalyzer::analyzeConstDecl(ConstDecl* decl) {
    analyzeExpression(decl->value.get());
    TypeInfo value_type = getExpressionType(decl->value.get());
    
    // Check for redefinition
    if (current_scope->symbols.find(decl->name) != current_scope->symbols.end()) {
        addError("Constant '" + decl->name + "' already defined", decl->line);
    }
    
    Symbol symbol(decl->name, value_type, true, false, decl->line);
    symbol.is_initialized = true;
    current_scope->defineSymbol(symbol);
}

void SemanticAnalyzer::analyzeFuncDecl(FuncDecl* decl) {
    // Build parameter types
    std::vector<TypeInfo> param_types;
    for (const auto& param : decl->parameters) {
        TypeInfo param_type = TypeInfo(GDType::VARIANT);
        if (!param.type.empty()) {
            param_type = getBuiltinType(param.type);
            if (param_type.base_type == GDType::UNKNOWN) {
                addError("Unknown parameter type '" + param.type + "'", decl->line);
            }
        }
        param_types.push_back(param_type);
    }
    
    // Get return type
    TypeInfo return_type = TypeInfo(GDType::VARIANT);
    if (!decl->return_type.empty()) {
        return_type = getBuiltinType(decl->return_type);
        if (return_type.base_type == GDType::UNKNOWN) {
            addError("Unknown return type '" + decl->return_type + "'", decl->line);
        }
    }
    
    // Check for redefinition
    if (current_scope->functions.find(decl->name) != current_scope->functions.end()) {
        addError("Function '" + decl->name + "' already defined", decl->line);
    }
    
    FunctionSignature signature(decl->name, param_types, return_type, decl->is_static, decl->line);
    current_scope->defineFunction(signature);
    
    // Analyze function body
    enterScope();
    
    std::string old_function = current_function;
    TypeInfo old_return_type = expected_return_type;
    
    current_function = decl->name;
    expected_return_type = return_type;
    
    // Add parameters to function scope
    for (size_t i = 0; i < decl->parameters.size(); ++i) {
        const auto& param = decl->parameters[i];
        Symbol param_symbol(param.name, param_types[i], false, false, decl->line);
        param_symbol.is_initialized = true;
        current_scope->defineSymbol(param_symbol);
    }
    
    analyzeStatement(decl->body.get());
    
    current_function = old_function;
    expected_return_type = old_return_type;
    
    exitScope();
}

void SemanticAnalyzer::analyzeClassDecl(ClassDecl* decl) {
    // Check for redefinition
    if (classes.find(decl->name) != classes.end()) {
        addError("Class '" + decl->name + "' already defined", decl->line);
    }
    
    ClassInfo class_info(decl->name, decl->base_class, decl->line);
    
    std::string old_class = current_class;
    current_class = decl->name;
    
    enterScope();
    
    // First pass: Register all function signatures and other declarations
    for (auto& member : decl->members) {
        if (member->type == ASTNodeType::FUNC_DECL) {
            FuncDecl* func_decl = static_cast<FuncDecl*>(member.get());
            
            // Build parameter types
            std::vector<TypeInfo> param_types;
            for (const auto& param : func_decl->parameters) {
                TypeInfo param_type = TypeInfo(GDType::VARIANT);
                if (!param.type.empty()) {
                    param_type = getBuiltinType(param.type);
                    if (param_type.base_type == GDType::UNKNOWN) {
                        addError("Unknown parameter type '" + param.type + "'", func_decl->line);
                    }
                }
                param_types.push_back(param_type);
            }
            
            // Get return type
            TypeInfo return_type = TypeInfo(GDType::VARIANT);
            if (!func_decl->return_type.empty()) {
                return_type = getBuiltinType(func_decl->return_type);
                if (return_type.base_type == GDType::UNKNOWN) {
                    addError("Unknown return type '" + func_decl->return_type + "'", func_decl->line);
                }
            }
            
            // Check for redefinition
            if (current_scope->functions.find(func_decl->name) != current_scope->functions.end()) {
                addError("Function '" + func_decl->name + "' already defined", func_decl->line);
            }
            
            FunctionSignature signature(func_decl->name, param_types, return_type, func_decl->is_static, func_decl->line);
            current_scope->defineFunction(signature);
            class_info.methods[func_decl->name] = signature;
        } else if (member->type == ASTNodeType::SIGNAL_DECL) {
            analyzeStatement(member.get());
            SignalDecl* signal_decl = static_cast<SignalDecl*>(member.get());
            class_info.signals.push_back(signal_decl->name);
        } else if (member->type == ASTNodeType::VAR_DECL || member->type == ASTNodeType::CONST_DECL || member->type == ASTNodeType::ENUM_DECL) {
            analyzeStatement(member.get());
            
            if (member->type == ASTNodeType::VAR_DECL) {
                VarDecl* var_decl = static_cast<VarDecl*>(member.get());
                TypeInfo member_type = getBuiltinType(var_decl->type);
                if (member_type.base_type == GDType::UNKNOWN && !var_decl->type.empty()) {
                    member_type = TypeInfo(GDType::VARIANT);
                }
                class_info.members[var_decl->name] = Symbol(var_decl->name, member_type, false, var_decl->is_static);
            }
        }
    }
    
    // Second pass: Analyze function bodies
    for (auto& member : decl->members) {
        if (member->type == ASTNodeType::FUNC_DECL) {
            FuncDecl* func_decl = static_cast<FuncDecl*>(member.get());
            
            // Get the already registered function signature
            auto* func_sig = current_scope->findFunction(func_decl->name);
            if (!func_sig) continue;
            
            // Analyze function body
            enterScope();
            
            std::string old_function = current_function;
            TypeInfo old_return_type = expected_return_type;
            
            current_function = func_decl->name;
            expected_return_type = func_sig->return_type;
            
            // Add parameters to function scope
            for (size_t i = 0; i < func_decl->parameters.size(); ++i) {
                const auto& param = func_decl->parameters[i];
                Symbol param_symbol(param.name, func_sig->parameter_types[i], false, false, func_decl->line);
                param_symbol.is_initialized = true;
                current_scope->defineSymbol(param_symbol);
            }
            
            analyzeStatement(func_decl->body.get());
            
            current_function = old_function;
            expected_return_type = old_return_type;
            
            exitScope();
        }
    }
    
    classes[decl->name] = class_info;
    
    current_class = old_class;
    exitScope();
}

void SemanticAnalyzer::analyzeSignalDecl(SignalDecl* decl) {
    // Check if signal name conflicts with existing symbols
    if (current_scope->findSymbol(decl->name)) {
        addError("Signal '" + decl->name + "' conflicts with existing symbol", decl->line);
        return;
    }
    
    // Signals are valid, just check parameter types
    for (const auto& param : decl->parameters) {
        if (!param.type.empty()) {
            TypeInfo param_type = getBuiltinType(param.type);
            if (param_type.base_type == GDType::UNKNOWN) {
                addError("Unknown signal parameter type '" + param.type + "'", decl->line);
            }
        }
    }
    
    // Register the signal name - signals are automatically initialized when declared
    Symbol signal_symbol(decl->name, TypeInfo(GDType::VARIANT), false, false, decl->line);
    signal_symbol.is_initialized = true;  // Signals are available immediately after declaration
    current_scope->defineSymbol(signal_symbol);
}

void SemanticAnalyzer::analyzeEnumDecl(EnumDecl* decl) {
    // Check if enum name conflicts with existing symbols
    if (current_scope->findSymbol(decl->name)) {
        addError("Enum '" + decl->name + "' conflicts with existing symbol", decl->line);
        return;
    }
    
    // Register the enum as a custom type
    TypeInfo enum_type(GDType::CUSTOM, decl->name);
    Symbol enum_symbol(decl->name, enum_type, true, false, decl->line);
    current_scope->defineSymbol(enum_symbol);
    
    // Analyze enum values
    int auto_value = 0;
    for (const auto& enum_value : decl->values) {
        // Check if enum value name conflicts
        if (current_scope->findSymbol(enum_value.name)) {
            addError("Enum value '" + enum_value.name + "' conflicts with existing symbol", decl->line);
            continue;
        }
        
        // If value has an explicit expression, analyze it
        if (enum_value.value) {
            analyzeExpression(enum_value.value.get());
            TypeInfo value_type = getExpressionType(enum_value.value.get());
            if (value_type.base_type != GDType::INT) {
                addError("Enum value '" + enum_value.name + "' must be an integer", decl->line);
            }
        }
        
        // Register the enum value as a constant
        Symbol value_symbol(enum_value.name, TypeInfo(GDType::INT), true, false, decl->line);
        current_scope->defineSymbol(value_symbol);
        
        auto_value++;
    }
}

void SemanticAnalyzer::analyzeBlockStmt(BlockStmt* stmt) {
    enterScope();
    
    for (auto& statement : stmt->statements) {
        analyzeStatement(statement.get());
    }
    
    exitScope();
}

void SemanticAnalyzer::analyzeIfStmt(IfStmt* stmt) {
    analyzeExpression(stmt->condition.get());
    TypeInfo condition_type = getExpressionType(stmt->condition.get());
    
    if (condition_type.base_type != GDType::BOOL && condition_type.base_type != GDType::VARIANT) {
        addWarning("Condition should be boolean, got " + condition_type.toString(), stmt->line);
    }
    
    analyzeStatement(stmt->then_branch.get());
    
    if (stmt->else_branch) {
        analyzeStatement(stmt->else_branch.get());
    }
}

void SemanticAnalyzer::analyzeWhileStmt(WhileStmt* stmt) {
    analyzeExpression(stmt->condition.get());
    TypeInfo condition_type = getExpressionType(stmt->condition.get());
    
    if (condition_type.base_type != GDType::BOOL && condition_type.base_type != GDType::VARIANT) {
        addWarning("Condition should be boolean, got " + condition_type.toString(), stmt->line);
    }
    
    bool old_in_loop = in_loop;
    in_loop = true;
    
    analyzeStatement(stmt->body.get());
    
    in_loop = old_in_loop;
}

void SemanticAnalyzer::analyzeForStmt(ForStmt* stmt) {
    analyzeExpression(stmt->iterable.get());
    TypeInfo iterable_type = getExpressionType(stmt->iterable.get());
    
    if (iterable_type.base_type != GDType::ARRAY && 
        iterable_type.base_type != GDType::STRING &&
        iterable_type.base_type != GDType::VARIANT) {
        addError("Cannot iterate over " + iterable_type.toString(), stmt->line);
    }
    
    enterScope();
    
    // Add loop variable
    TypeInfo loop_var_type = TypeInfo(GDType::VARIANT);
    if (iterable_type.base_type == GDType::STRING) {
        loop_var_type = TypeInfo(GDType::STRING);
    }
    
    Symbol loop_var(stmt->variable, loop_var_type, false, false, stmt->line);
    loop_var.is_initialized = true;
    current_scope->defineSymbol(loop_var);
    
    bool old_in_loop = in_loop;
    in_loop = true;
    
    analyzeStatement(stmt->body.get());
    
    in_loop = old_in_loop;
    exitScope();
}

void SemanticAnalyzer::analyzeReturnStmt(ReturnStmt* stmt) {
    if (current_function.empty()) {
        addError("Return statement outside of function", stmt->line);
        return;
    }
    
    TypeInfo return_type = TypeInfo(GDType::VOID);
    
    if (stmt->value) {
        analyzeExpression(stmt->value.get());
        return_type = getExpressionType(stmt->value.get());
    }
    
    if (!expected_return_type.isCompatibleWith(return_type)) {
        addError("Return type mismatch: expected " + expected_return_type.toString() + 
                ", got " + return_type.toString(), stmt->line);
    }
}

void SemanticAnalyzer::analyzeExpressionStmt(ExpressionStmt* stmt) {
    analyzeExpression(stmt->expression.get());
}

void SemanticAnalyzer::analyzeExpression(Expression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case ASTNodeType::LITERAL:
            analyzeLiteralExpr(static_cast<LiteralExpr*>(expr));
            break;
        case ASTNodeType::IDENTIFIER:
            analyzeIdentifierExpr(static_cast<IdentifierExpr*>(expr));
            break;
        case ASTNodeType::BINARY_OP:
            analyzeBinaryOpExpr(static_cast<BinaryOpExpr*>(expr));
            break;
        case ASTNodeType::UNARY_OP:
            analyzeUnaryOpExpr(static_cast<UnaryOpExpr*>(expr));
            break;
        case ASTNodeType::CALL:
            analyzeCallExpr(static_cast<CallExpr*>(expr));
            break;
        case ASTNodeType::MEMBER_ACCESS:
            analyzeMemberAccessExpr(static_cast<MemberAccessExpr*>(expr));
            break;
        case ASTNodeType::ARRAY_ACCESS:
            analyzeArrayAccessExpr(static_cast<ArrayAccessExpr*>(expr));
            break;
        case ASTNodeType::ARRAY_LITERAL:
            analyzeArrayLiteralExpr(static_cast<ArrayLiteralExpr*>(expr));
            break;
        case ASTNodeType::DICT_LITERAL:
            analyzeDictLiteralExpr(static_cast<DictLiteralExpr*>(expr));
            break;
        case ASTNodeType::LAMBDA:
            analyzeLambdaExpr(static_cast<LambdaExpr*>(expr));
            break;
        case ASTNodeType::TERNARY:
            analyzeTernaryExpr(static_cast<TernaryExpr*>(expr));
            break;
        default:
            addError("Unknown expression type", expr->line);
            break;
    }
}

void SemanticAnalyzer::analyzeLiteralExpr(LiteralExpr* expr) {
    // Literals are always valid
}

void SemanticAnalyzer::analyzeIdentifierExpr(IdentifierExpr* expr) {
    Symbol* symbol = current_scope->findSymbol(expr->name);
    FunctionSignature* function = current_scope->findFunction(expr->name);
    
    if (!symbol && !function) {
        addError("Undefined variable '" + expr->name + "'", expr->line);
    } else if (symbol && !symbol->is_initialized) {
        addWarning("Variable '" + expr->name + "' used before initialization", expr->line);
    }
}

void SemanticAnalyzer::analyzeBinaryOpExpr(BinaryOpExpr* expr) {
    analyzeExpression(expr->left.get());
    analyzeExpression(expr->right.get());
    
    TypeInfo left_type = getExpressionType(expr->left.get());
    TypeInfo right_type = getExpressionType(expr->right.get());
    
    // Get the result type to check if the operation is valid
    TypeInfo result_type = getBinaryResultType(left_type, expr->operator_type, right_type);
    if (result_type.base_type == GDType::UNKNOWN) {
        addError("Type mismatch in binary operation: " + left_type.toString() + 
                " and " + right_type.toString(), expr->line);
    }
}

void SemanticAnalyzer::analyzeUnaryOpExpr(UnaryOpExpr* expr) {
    analyzeExpression(expr->operand.get());
    TypeInfo operand_type = getExpressionType(expr->operand.get());
    
    TypeInfo result_type = getUnaryResultType(expr->operator_type, operand_type);
    if (result_type.base_type == GDType::UNKNOWN) {
        addError("Invalid unary operation on " + operand_type.toString(), expr->line);
    }
}

void SemanticAnalyzer::analyzeCallExpr(CallExpr* expr) {
    // Analyze arguments first
    for (auto& arg : expr->arguments) {
        analyzeExpression(arg.get());
    }
    
    // Check if it's a function call
    if (expr->callee->type == ASTNodeType::IDENTIFIER) {
        IdentifierExpr* id_expr = static_cast<IdentifierExpr*>(expr->callee.get());
        FunctionSignature* func = current_scope->findFunction(id_expr->name);
        
        if (func) {
            // Handle variadic functions (like print)
            if (func->is_variadic) {
                // Variadic functions accept any number of arguments
                // Just validate that all arguments are valid expressions
                // (already done above in the loop)
            } else {
                // Check argument count and types for non-variadic functions
                if (expr->arguments.size() != func->parameter_types.size()) {
                    addError("Function '" + func->name + "' expects " + 
                            std::to_string(func->parameter_types.size()) + " arguments, got " + 
                            std::to_string(expr->arguments.size()), expr->line);
                } else {
                    for (size_t i = 0; i < expr->arguments.size(); ++i) {
                        TypeInfo arg_type = getExpressionType(expr->arguments[i].get());
                        if (!func->parameter_types[i].isCompatibleWith(arg_type)) {
                            addError("Argument " + std::to_string(i + 1) + " type mismatch: expected " + 
                                    func->parameter_types[i].toString() + ", got " + arg_type.toString(), expr->line);
                        }
                    }
                }
            }
        } else {
            // Check if it's a variable that might be callable
            analyzeExpression(expr->callee.get());
        }
    } else {
        // For non-identifier callees (like member access), analyze normally
        analyzeExpression(expr->callee.get());
    }
}

void SemanticAnalyzer::analyzeMemberAccessExpr(MemberAccessExpr* expr) {
    analyzeExpression(expr->object.get());
    // Member access validation would require more detailed type information
}

void SemanticAnalyzer::analyzeArrayAccessExpr(ArrayAccessExpr* expr) {
    analyzeExpression(expr->array.get());
    analyzeExpression(expr->index.get());
    
    TypeInfo array_type = getExpressionType(expr->array.get());
    TypeInfo index_type = getExpressionType(expr->index.get());
    
    if (array_type.base_type != GDType::ARRAY && 
        array_type.base_type != GDType::STRING &&
        array_type.base_type != GDType::DICTIONARY &&
        array_type.base_type != GDType::VARIANT) {
        addError("Cannot index " + array_type.toString(), expr->line);
    }
    
    if (array_type.base_type == GDType::ARRAY || array_type.base_type == GDType::STRING) {
        if (index_type.base_type != GDType::INT && index_type.base_type != GDType::VARIANT) {
            addError("Array/String index must be integer, got " + index_type.toString(), expr->line);
        }
    }
}

void SemanticAnalyzer::analyzeArrayLiteralExpr(ArrayLiteralExpr* expr) {
    for (auto& element : expr->elements) {
        analyzeExpression(element.get());
    }
}

void SemanticAnalyzer::analyzeDictLiteralExpr(DictLiteralExpr* expr) {
    for (auto& pair : expr->pairs) {
        analyzeExpression(pair.first.get());
        analyzeExpression(pair.second.get());
    }
}

TypeInfo SemanticAnalyzer::getExpressionType(Expression* expr) {
    if (!expr) return TypeInfo(GDType::UNKNOWN);
    
    switch (expr->type) {
        case ASTNodeType::LITERAL: {
            LiteralExpr* lit = static_cast<LiteralExpr*>(expr);
            switch (lit->literal_type) {
                case TokenType::INTEGER: return TypeInfo(GDType::INT);
                case TokenType::FLOAT: return TypeInfo(GDType::FLOAT);
                case TokenType::STRING: return TypeInfo(GDType::STRING);
                case TokenType::BOOLEAN: return TypeInfo(GDType::BOOL);
                case TokenType::NULL_LITERAL: return TypeInfo(GDType::VARIANT);
                default: return TypeInfo(GDType::UNKNOWN);
            }
        }
        case ASTNodeType::IDENTIFIER: {
            IdentifierExpr* id = static_cast<IdentifierExpr*>(expr);
            Symbol* symbol = current_scope->findSymbol(id->name);
            if (symbol) {
                return symbol->type;
            }
            FunctionSignature* function = current_scope->findFunction(id->name);
            if (function) {
                return TypeInfo(GDType::LAMBDA); // Functions can be treated as callable objects
            }
            return TypeInfo(GDType::UNKNOWN);
        }
        case ASTNodeType::BINARY_OP: {
            BinaryOpExpr* bin = static_cast<BinaryOpExpr*>(expr);
            TypeInfo left_type = getExpressionType(bin->left.get());
            TypeInfo right_type = getExpressionType(bin->right.get());
            return getBinaryResultType(left_type, bin->operator_type, right_type);
        }
        case ASTNodeType::UNARY_OP: {
            UnaryOpExpr* un = static_cast<UnaryOpExpr*>(expr);
            TypeInfo operand_type = getExpressionType(un->operand.get());
            return getUnaryResultType(un->operator_type, operand_type);
        }
        case ASTNodeType::CALL: {
            CallExpr* call = static_cast<CallExpr*>(expr);
            if (call->callee->type == ASTNodeType::IDENTIFIER) {
                IdentifierExpr* id = static_cast<IdentifierExpr*>(call->callee.get());
                FunctionSignature* func = current_scope->findFunction(id->name);
                return func ? func->return_type : TypeInfo(GDType::UNKNOWN);
            }
            return TypeInfo(GDType::VARIANT);
        }
        case ASTNodeType::ARRAY_LITERAL:
            return TypeInfo(GDType::ARRAY);
        case ASTNodeType::DICT_LITERAL:
            return TypeInfo(GDType::DICTIONARY);
        case ASTNodeType::LAMBDA:
            return TypeInfo(GDType::LAMBDA);
        case ASTNodeType::TERNARY: {
            TernaryExpr* ternary = static_cast<TernaryExpr*>(expr);
            TypeInfo true_type = getExpressionType(ternary->true_expr.get());
            TypeInfo false_type = getExpressionType(ternary->false_expr.get());
            
            // If both branches have the same type, return that type
            if (true_type == false_type) {
                return true_type;
            }
            
            // If one is variant, return the other
            if (true_type.base_type == GDType::VARIANT) {
                return false_type;
            }
            if (false_type.base_type == GDType::VARIANT) {
                return true_type;
            }
            
            // If both are numeric, return the more general type
            if (true_type.isNumeric() && false_type.isNumeric()) {
                return (true_type.base_type == GDType::FLOAT || false_type.base_type == GDType::FLOAT) ?
                       TypeInfo(GDType::FLOAT) : TypeInfo(GDType::INT);
            }
            
            // Otherwise, return variant as a fallback
            return TypeInfo(GDType::VARIANT);
        }
        default:
            return TypeInfo(GDType::VARIANT);
    }
}

bool SemanticAnalyzer::areTypesCompatible(const TypeInfo& left, const TypeInfo& right, TokenType op) {
    // Variant is compatible with everything
    if (left.base_type == GDType::VARIANT || right.base_type == GDType::VARIANT) {
        return true;
    }
    
    switch (op) {
        case TokenType::PLUS:
            // String concatenation or numeric addition
            if (left.base_type == GDType::STRING || right.base_type == GDType::STRING) {
                return true;
            }
            return left.isNumeric() && right.isNumeric();
            
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
            return left.isNumeric() && right.isNumeric();
            
        case TokenType::MODULO:
            // String formatting: "format string" % [args]
            if (left.base_type == GDType::STRING && right.base_type == GDType::ARRAY) {
                return true;
            }
            // Regular modulo operation
            return left.isNumeric() && right.isNumeric();
            
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
            return true; // Any types can be compared for equality
            
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            return (left.isNumeric() && right.isNumeric()) || 
                   (left.base_type == GDType::STRING && right.base_type == GDType::STRING);
            
        case TokenType::AND:
        case TokenType::OR:
            return true; // Any types can be used in logical operations
            
        default:
            return false;
    }
}

TypeInfo SemanticAnalyzer::getUnaryResultType(TokenType op, const TypeInfo& operand) {
    switch (op) {
        case TokenType::MINUS:
        case TokenType::PLUS:
            return operand.isNumeric() ? operand : TypeInfo(GDType::UNKNOWN);
        case TokenType::NOT:
        case TokenType::LOGICAL_NOT:
            return TypeInfo(GDType::BOOL);
        default:
            return TypeInfo(GDType::UNKNOWN);
    }
}

TypeInfo SemanticAnalyzer::getBinaryResultType(const TypeInfo& left, TokenType op, const TypeInfo& right) {
    // Variant propagates
    if (left.base_type == GDType::VARIANT || right.base_type == GDType::VARIANT) {
        return TypeInfo(GDType::VARIANT);
    }
    
    switch (op) {
        case TokenType::PLUS:
            if (left.base_type == GDType::STRING || right.base_type == GDType::STRING) {
                return TypeInfo(GDType::STRING);
            }
            if (left.isNumeric() && right.isNumeric()) {
                return (left.base_type == GDType::FLOAT || right.base_type == GDType::FLOAT) ? 
                       TypeInfo(GDType::FLOAT) : TypeInfo(GDType::INT);
            }
            break;
            
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
            if (left.isNumeric() && right.isNumeric()) {
                return (left.base_type == GDType::FLOAT || right.base_type == GDType::FLOAT) ? 
                       TypeInfo(GDType::FLOAT) : TypeInfo(GDType::INT);
            }
            break;
            
        case TokenType::MODULO:
            // String formatting: "format string" % [args]
            if (left.base_type == GDType::STRING && right.base_type == GDType::ARRAY) {
                return TypeInfo(GDType::STRING);
            }
            // Regular modulo operation
            if (left.isNumeric() && right.isNumeric()) {
                return (left.base_type == GDType::FLOAT || right.base_type == GDType::FLOAT) ? 
                       TypeInfo(GDType::FLOAT) : TypeInfo(GDType::INT);
            }
            break;
            
        // Assignment operators return the left operand type
        case TokenType::ASSIGN:
        case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN:
        case TokenType::MULTIPLY_ASSIGN:
        case TokenType::DIVIDE_ASSIGN:
        case TokenType::MODULO_ASSIGN:
            // For assignments, check if right side is compatible with left
            if (right.isCompatibleWith(left)) {
                return left;
            }
            break;
            
        // Type inference assignment - return the right operand type
        case TokenType::TYPE_INFER_ASSIGN:
            return right;
            
        // Comparison operators return bool
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            // Allow comparison between compatible types
            if ((left.isNumeric() && right.isNumeric()) ||
                (left.base_type == GDType::STRING && right.base_type == GDType::STRING) ||
                left.base_type == GDType::VARIANT || right.base_type == GDType::VARIANT) {
                return TypeInfo(GDType::BOOL);
            }
            break;
            
        // Logical operators
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::LOGICAL_AND:
        case TokenType::LOGICAL_OR:
            return TypeInfo(GDType::BOOL);
            
        default:
            break;
    }
    
    return TypeInfo(GDType::UNKNOWN);
}