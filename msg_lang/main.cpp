#include "parser.h"
#include "module.h"
#include "cpp_gen.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <variant>

using namespace pt::msg_lang;

struct Arguments {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path headerOut;
    std::filesystem::path sourceOut;
    std::vector<std::string> includeHeaders;
    std::vector<std::string> systemHeaders;
};

constexpr std::string_view kUsage = R"##(
Usage: msg_lang_c -h HEADER_OUTPUT -s SOURCE_OUTPUT -i INCLUDE_HEADER --system_header SYSTEM_HEADER INPUT_FILES...
    -i               INCLUDE_HEADER      #include'd in HEADER_OUTPUT
    --system_header  INCLUDE_HEADER      #include'd in HEADER_OUTPUT with <>
)##";

// if string then string contains the error
std::variant<Arguments, std::string> parseArgs(int argc, char** argv) {
    enum class NextArg {
        input,
        headerOut,
        sourceOut,
        includeHeader,
        systemHeader,
    };

    Arguments rtn;
    NextArg nextArg = NextArg::input;
    for (int i = 1; i < argc; i++) {
        std::string_view arg(argv[i]);

        switch (nextArg) {
            case NextArg::input: {
                if (arg == "-h") {
                    nextArg = NextArg::headerOut;
                } else if (arg == "-s") {
                    nextArg = NextArg::sourceOut;
                } else if (arg == "-i") {
                    nextArg = NextArg::includeHeader;
                } else if (arg == "--system_header") {
                    nextArg = NextArg::systemHeader;
                } else {
                    rtn.inputs.push_back(arg);
                }
                break;
            }
            case NextArg::headerOut: {
                rtn.headerOut = arg;
                nextArg = NextArg::input;
                break;
            }
            case NextArg::sourceOut: {
                rtn.sourceOut = arg;
                nextArg = NextArg::input;
                break;
            }
            case NextArg::includeHeader: {
                rtn.includeHeaders.push_back(std::string(arg));
                nextArg = NextArg::input;
                break;
            }
            case NextArg::systemHeader: {
                rtn.systemHeaders.push_back(std::string(arg));
                nextArg = NextArg::input;
                break;
            }
        }
    }

    if (rtn.headerOut == std::filesystem::path()) {
        return "No header output path specified, use -h";
    } else if (rtn.sourceOut == std::filesystem::path()) {
        return "No source output path specified, use -s";
    } else if (rtn.inputs.empty()) {
        return "No inputs specified";
    } else {
        return rtn;
    }
}

int main(int argc, char** argv) {
    const auto args_result = parseArgs(argc, argv);
    if (std::holds_alternative<std::string>(args_result)) {
        std::cerr << std::get<std::string>(args_result) << std::endl;
        std::cerr << kUsage << std::endl;
        return 1;
    }

    assert(std::holds_alternative<Arguments>(args_result));
    const Arguments& args = std::get<Arguments>(args_result);
    const auto& inputs = args.inputs;
    const auto& headerOut = args.headerOut;
    const auto& sourceOut = args.sourceOut;

    bool allInputsExist = true;
    for (const auto& path: inputs) {
        if (!std::filesystem::exists(path)) {
            std::cerr << "Path " << path << " does not exist" << std::endl;
            allInputsExist = false;
        }
    }

    if (!allInputsExist) {
        return 1;
    }

    std::vector<AstFile> asts;
    for (const auto& path: inputs) {
        asts.push_back(parse(path));
    }

    {
        bool hadErrors = false;
        for (const auto& ast: asts) {
            if (!ast.errors.empty()) {
                hadErrors = true;
                for (const auto& error: ast.errors) {
                    std::cerr << formatError(error) << std::endl;
                }
            }
        }

        if (hadErrors) {
            return 1;
        }
    }
    module::Module mod = module::compile(asts);

    if (!mod.errors.empty()) {
        for (const auto& error: mod.errors) {
            std::cerr << formatError(error) << std::endl;
        }
        return 1;
    }

    CppSource output = genCpp(mod, args.includeHeaders, args.systemHeaders);

    {
        std::ofstream f(headerOut);
        if (f) {
            f.write(output.header.data(), output.header.size());
        }

        if (!f.good()) {
            std::cerr << "Failed to write to header file" << std::endl;
            return 2;
        }
    }

    {
        std::ofstream f(sourceOut);
        if (f) {
            f.write(output.source.data(), output.source.size());
        }

        if (!f.good()) {
            std::cerr << "Failed to write to source file" << std::endl;
            return 2;
        }   
    }

    return 0;
}