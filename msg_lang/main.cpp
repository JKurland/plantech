#include "parser.h"
#include "module.h"
#include "cpp_gen.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace pt::msg_lang;

int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }


    std::vector<std::filesystem::path> inputs;
    std::filesystem::path outputPath;

    bool nextArgIsOutputPath = false;
    for (const auto& arg: args) {
        if (arg.starts_with("-o")) {
            if (arg.size() == 2) {
                nextArgIsOutputPath = true;
            } else {
                std::string_view outputPathStr = arg;
                outputPathStr.remove_prefix(2);
                outputPath = outputPathStr;
            }
        } else {
            if (nextArgIsOutputPath) {
                nextArgIsOutputPath = false;
                outputPath = arg;
            } else {
                inputs.push_back(arg);
            }
        }
    }

    if (outputPath == std::filesystem::path()) {
        std::cout << "Specify an output path with -o" << std::endl;
        return -1;
    }

    if (inputs.empty()) {
        std::cout << "Specify input file(s)" << std::endl;
        return -1;
    }


    for (const auto& path: inputs) {
        if (!std::filesystem::exists(path)) {
            std::cout << "Path " << path << " does not exist" << std::endl;
        }
    }

    std::vector<AstFile> asts;
    for (const auto& path: inputs) {
        asts.push_back(parse(path));
    }


    bool hadErrors = false;
    for (const auto& ast: asts) {
        if (!ast.errors.empty()) {
            hadErrors = true;
            for (const auto& error: ast.errors) {
                std::cout << formatError(error) << std::endl;
            }
        }
    }

    if (hadErrors) {
        return 1;
    }

    module::Module mod = module::compile(asts);

    std::string output = genCpp(mod);

    std::ofstream outFile(outputPath);
    if (outFile) {
        outFile.write(output.data(), output.size());
    }

    if (!outFile.good()) {
        std::cout << "Failed to write to output file" << std::endl;
        return 2;
    }

    std::cout << "Compilation Successful" << std::endl;
}