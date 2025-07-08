#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "code_generator.h"

class GDScriptCompiler {
public:
    bool compile(const std::string& source_file, const std::string& output_file, 
                TargetPlatform platform = TargetPlatform::MACOS_X64, 
                OutputFormat format = OutputFormat::OBJECT) {
        try {
            // Read source file
            std::ifstream file(source_file);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open source file: " << source_file << std::endl;
                return false;
            }
            
            std::string source_code((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
            file.close();
            
            // Lexical Analysis
            std::cout << "[1/4] Lexical Analysis..." << std::endl;
            Lexer lexer(source_code);
            auto tokens = lexer.tokenize();
            
            if (lexer.hasErrors()) {
                std::cerr << "Lexical analysis failed." << std::endl;
                return false;
            }
            
            // Debug: Print first few tokens only
            std::cout << "Tokens generated: " << tokens.size() << std::endl;
            
            // Syntax Analysis
            std::cout << "[2/4] Syntax Analysis..." << std::endl;
            Parser parser(tokens);
            auto ast = parser.parse();
            
            if (parser.hasErrors()) {
                std::cerr << "Syntax analysis failed." << std::endl;
                return false;
            }
            
            // Semantic Analysis
            std::cout << "[3/4] Semantic Analysis..." << std::endl;
            SemanticAnalyzer analyzer;
            analyzer.analyze(ast.get());
            
            if (analyzer.hasErrors()) {
                std::cerr << "Semantic analysis failed." << std::endl;
                return false;
            }
            
            // Code Generation
            std::cout << "[4/4] Code Generation..." << std::endl;
            CodeGenerator generator(platform, format);
            generator.generate(ast.get(), output_file, &analyzer);
            
            if (generator.hasErrors()) {
                std::cerr << "Code generation failed." << std::endl;
                return false;
            }
            
            std::cout << "Compilation successful! Output: " << output_file << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Compilation error: " << e.what() << std::endl;
            return false;
        }
    }
};

TargetPlatform parseTargetPlatform(const std::string& platform_str) {
    if (platform_str == "windows" || platform_str == "win64") return TargetPlatform::WINDOWS_X64;
    if (platform_str == "macos" || platform_str == "mac64") return TargetPlatform::MACOS_X64;
    if (platform_str == "macos-arm" || platform_str == "mac-arm") return TargetPlatform::MACOS_ARM64;
    if (platform_str == "linux" || platform_str == "linux64") return TargetPlatform::LINUX_X64;
    if (platform_str == "linux-arm" || platform_str == "linux-arm64") return TargetPlatform::LINUX_ARM64;
    return TargetPlatform::MACOS_X64; // default
}

OutputFormat parseOutputFormat(const std::string& format_str) {
    if (format_str == "asm" || format_str == "assembly") return OutputFormat::ASSEMBLY;
    if (format_str == "obj" || format_str == "object") return OutputFormat::OBJECT;
    if (format_str == "exe" || format_str == "executable") return OutputFormat::EXECUTABLE;
    return OutputFormat::OBJECT; // default
}

void printUsage(const char* program_name) {
    std::cout << "GDScript Compiler v1.0 - Cross-Platform Edition" << std::endl;
    std::cout << "Usage: " << program_name << " <input.gd> <output> [options]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --platform <target>    Target platform (windows, macos, macos-arm, linux, linux-arm)" << std::endl;
    std::cout << "  --format <format>      Output format (assembly, object, executable)" << std::endl;
    std::cout << "  --help                 Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << " player.gd player.gdc" << std::endl;
    std::cout << "  " << program_name << " player.gd player --platform windows --format executable" << std::endl;
    std::cout << "  " << program_name << " player.gd player.exe --platform linux --format executable" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    TargetPlatform platform = TargetPlatform::MACOS_X64;
    OutputFormat format = OutputFormat::OBJECT;
    
    // Parse command line arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--platform" && i + 1 < argc) {
            platform = parseTargetPlatform(argv[++i]);
        }
        else if (arg == "--format" && i + 1 < argc) {
            format = parseOutputFormat(argv[++i]);
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    GDScriptCompiler compiler;
    bool success = compiler.compile(input_file, output_file, platform, format);
    
    if (success) {
        std::cout << "Target Platform: " << CodeGenerator(platform, format).getPlatformName() << std::endl;
        std::cout << "Output Format: ";
        switch (format) {
            case OutputFormat::ASSEMBLY: std::cout << "Assembly"; break;
            case OutputFormat::OBJECT: std::cout << "Object File"; break;
            case OutputFormat::EXECUTABLE: std::cout << "Executable"; break;
        }
        std::cout << std::endl;
    }
    
    return success ? 0 : 1;
}