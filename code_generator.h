#pragma once

#include "parser.h"
#include "semantic_analyzer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>

// Forward declarations
class Register;
class Instruction;
class BasicBlock;
class Function;

// Register allocation
class Register {
public:
    enum Type {
        GENERAL,    // General purpose register
        FLOAT,      // Floating point register
        VIRTUAL     // Virtual register (before allocation)
    };
    
    int id;
    Type type;
    bool is_allocated;
    std::string name;
    
    Register(int id, Type type, const std::string& name = "")
        : id(id), type(type), is_allocated(false), name(name) {}
};

// Instruction representation
class Instruction {
public:
    enum OpCode {
        // Data movement
        MOV, LOAD, STORE,
        
        // Arithmetic
        ADD, SUB, MUL, DIV, MOD,
        FADD, FSUB, FMUL, FDIV,
        
        // Logical
        AND, OR, XOR, NOT,
        
        // Comparison
        CMP, FCMP,
        
        // Branching
        JMP, JE, JNE, JL, JLE, JG, JGE,
        
        // Function calls
        CALL, RET,
        
        // Stack operations
        PUSH, POP,
        
        // Special
        NOP, LABEL
    };
    
    OpCode opcode;
    std::vector<std::shared_ptr<Register>> operands;
    std::string label;  // For labels and jumps
    int immediate;      // For immediate values
    bool has_immediate;
    
    Instruction(OpCode op) : opcode(op), immediate(0), has_immediate(false) {}
    Instruction(OpCode op, const std::string& lbl) : opcode(op), label(lbl), immediate(0), has_immediate(false) {}
    
    std::string toString() const;
};

// Basic block for control flow
class BasicBlock {
public:
    std::string label;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;
    
    BasicBlock(const std::string& label) : label(label) {}
    
    void addInstruction(std::unique_ptr<Instruction> instr);
    void addSuccessor(BasicBlock* block);
};

// Function representation
class Function {
public:
    std::string name;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::vector<std::shared_ptr<Register>> parameters;
    std::shared_ptr<Register> return_register;
    int stack_size;
    
    Function(const std::string& name) : name(name), stack_size(0) {}
    
    BasicBlock* createBlock(const std::string& label);
    BasicBlock* getBlock(const std::string& label);
};

// Target platform enumeration
enum class TargetPlatform {
    WINDOWS_X64,
    MACOS_X64,
    MACOS_ARM64,
    LINUX_X64,
    LINUX_ARM64
};

// Output format enumeration
enum class OutputFormat {
    ASSEMBLY,     // .s files
    OBJECT,       // .o files
    EXECUTABLE    // .exe/.app/binary files
};

// Code generator class
class CodeGenerator {
private:
    std::vector<std::unique_ptr<Function>> functions;
    std::unordered_map<std::string, std::shared_ptr<Register>> variables;
    std::unordered_map<std::string, std::shared_ptr<Register>> class_members;
    std::unordered_map<std::string, Function*> function_map;
    std::string current_class_name;
    
    // Target platform and output format
    TargetPlatform target_platform;
    OutputFormat output_format;
    
    Function* current_function;
    BasicBlock* current_block;
    
    int next_register_id;
    int next_label_id;
    int stack_offset;
    
    std::vector<std::string> errors;
    
    // Register allocation
    std::vector<std::shared_ptr<Register>> available_registers;
    std::vector<std::shared_ptr<Register>> allocated_registers;
    
    // Built-in function declarations
    std::unordered_map<std::string, std::string> builtin_functions;
    
    // Semantic analyzer reference
    SemanticAnalyzer* semantic_analyzer;
    
public:
    CodeGenerator();
    CodeGenerator(SemanticAnalyzer* analyzer);
    CodeGenerator(TargetPlatform platform, OutputFormat format = OutputFormat::ASSEMBLY);
    CodeGenerator(SemanticAnalyzer* analyzer, TargetPlatform platform, OutputFormat format = OutputFormat::ASSEMBLY);
    
    // Main generation methods
    bool generate(ASTNode* root, const std::string& output_file);
    bool generate(ASTNode* root, const std::string& output_file, SemanticAnalyzer* analyzer);
    bool generate(ASTNode* root, const std::string& output_file, TargetPlatform platform, OutputFormat format = OutputFormat::ASSEMBLY);
    void generateProgram(Program* program);
    
    // Platform and format configuration
    void setTargetPlatform(TargetPlatform platform);
    void setOutputFormat(OutputFormat format);
    TargetPlatform getTargetPlatform() const;
    OutputFormat getOutputFormat() const;
    std::string getPlatformName() const;
    std::string getExecutableExtension() const;
    
    // Statement generation
    void generateStatement(Statement* stmt);
    void generateVarDecl(VarDecl* decl);
    void generateConstDecl(ConstDecl* decl);
    void generateFuncDecl(FuncDecl* decl);
    void generateClassDecl(ClassDecl* decl);
    void generateSignalDecl(SignalDecl* decl);
    void generateEnumDecl(EnumDecl* decl);
    void generateBlockStmt(BlockStmt* stmt);
    void generateIfStmt(IfStmt* stmt);
    void generateWhileStmt(WhileStmt* stmt);
    void generateForStmt(ForStmt* stmt);
    void generateReturnStmt(ReturnStmt* stmt);
    void generateExpressionStmt(ExpressionStmt* stmt);
    void generateBreakStmt(BreakStmt* stmt);
    void generateContinueStmt(ContinueStmt* stmt);
    void generateMatchStmt(MatchStmt* stmt);
    
    // Expression generation
    std::shared_ptr<Register> generateExpression(Expression* expr);
    std::shared_ptr<Register> generateLiteralExpr(LiteralExpr* expr);
    std::shared_ptr<Register> generateIdentifierExpr(IdentifierExpr* expr);
    std::shared_ptr<Register> generateBinaryOpExpr(BinaryOpExpr* expr);
    std::shared_ptr<Register> generateUnaryOpExpr(UnaryOpExpr* expr);
    std::shared_ptr<Register> generateCallExpr(CallExpr* expr);
    std::shared_ptr<Register> generateMemberAccessExpr(MemberAccessExpr* expr);
    std::shared_ptr<Register> generateArrayAccessExpr(ArrayAccessExpr* expr);
    std::shared_ptr<Register> generateArrayLiteralExpr(ArrayLiteralExpr* expr);
    std::shared_ptr<Register> generateDictLiteralExpr(DictLiteralExpr* expr);
    std::shared_ptr<Register> generateLambdaExpr(LambdaExpr* expr);
    std::shared_ptr<Register> generateTernaryExpr(TernaryExpr* expr);
    
    // Register management
    std::shared_ptr<Register> allocateRegister(Register::Type type = Register::GENERAL);
    std::shared_ptr<Register> allocateVirtualRegister(Register::Type type = Register::GENERAL);
    void freeRegister(std::shared_ptr<Register> reg);
    void performRegisterAllocation();
    
    // Label management
    std::string generateLabel(const std::string& prefix = "L");
    
    // Instruction helpers
    void emit(Instruction::OpCode opcode);
    void emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest);
    void emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, std::shared_ptr<Register> src);
    void emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, std::shared_ptr<Register> src1, std::shared_ptr<Register> src2);
    void emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, int immediate);
    void emit(Instruction::OpCode opcode, const std::string& label);
    void emitLabel(const std::string& label);
    
    // Type conversion helpers
    std::shared_ptr<Register> convertType(std::shared_ptr<Register> src, GDType from_type, GDType to_type);
    
    // Built-in function support
    void initializeBuiltinFunctions();
    bool isBuiltinFunction(const std::string& name);
    std::shared_ptr<Register> generateBuiltinCall(const std::string& name, const std::vector<std::shared_ptr<Register>>& args);
    
    // Assembly output
    void writeAssembly(const std::string& filename);
    void writeObjectFile(const std::string& filename);
    void writeExecutable(const std::string& filename);
    
    // Cross-platform executable generation
    void generateWindowsExecutable(const std::string& filename);
    void generateMacOSExecutable(const std::string& filename);
    void generateLinuxExecutable(const std::string& filename);
    
    // Platform-specific assembly generation
    void writeWindowsAssembly(const std::string& filename);
    void writeMacOSAssembly(const std::string& filename);
    void writeLinuxAssembly(const std::string& filename);
    
    // Runtime system generation
    void generateRuntimeLibrary();
    void generateStartupCode();
    void generateShutdownCode();
    
    // Linking support
    bool linkExecutable(const std::string& object_file, const std::string& executable_file);
    std::vector<std::string> getPlatformLibraries() const;
    std::string getLinkerCommand() const;
    
    // Error handling
    void addError(const std::string& message);
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    
    // Utility methods
    void optimizeCode();
    void performDeadCodeElimination();
    void performConstantFolding();
    
    // Platform-specific code generation
    std::string getArchitecture() const;
    void generatePlatformSpecificCode();
    
    // Debug information
    void generateDebugInfo(const std::string& source_file);
    
    // Machine code generation
    std::vector<uint8_t> generateMachineCode();
    std::vector<uint8_t> generateInstructionBytes(Instruction* instr);
    std::vector<uint8_t> generateX86_64Instruction(Instruction* instr);
    std::vector<uint8_t> generateARM64Instruction(Instruction* instr);
    
private:
    // Helper methods
    void setupFunction(const std::string& name);
    void finalizeFunction();
    
    // Control flow helpers
    std::vector<std::string> break_labels;
    std::vector<std::string> continue_labels;
    
    void pushBreakLabel(const std::string& label) { break_labels.push_back(label); }
    void pushContinueLabel(const std::string& label) { continue_labels.push_back(label); }
    void popBreakLabel() { if (!break_labels.empty()) break_labels.pop_back(); }
    void popContinueLabel() { if (!continue_labels.empty()) continue_labels.pop_back(); }
    std::string getCurrentBreakLabel() { return break_labels.empty() ? "" : break_labels.back(); }
    std::string getCurrentContinueLabel() { return continue_labels.empty() ? "" : continue_labels.back(); }
    
    // Memory management
    void generateGarbageCollector();
    void generateMemoryAllocation(std::shared_ptr<Register> size_reg);
    void generateMemoryDeallocation(std::shared_ptr<Register> ptr_reg);
    
    // Runtime support
    void generateRuntimeSupport();
    void generateTypeChecking();
    void generateStringOperations();
    void generateArrayOperations();
    void generateDictionaryOperations();
};