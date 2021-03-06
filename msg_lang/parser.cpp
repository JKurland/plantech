#include "parser.h"

#include <string_view>
#include <variant>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <span>
#include <iostream>


namespace pt::msg_lang {

struct TokenisedFile {
    SourceFile sourceFile;
    
    // tokens and errors will have string_views pointing into sourceFile
    std::vector<Token> tokens;
    std::vector<Error> errors;
};

bool isWhiteSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isAlpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool isAlphaNum(char c) {
    return isAlpha(c) || ('0' <= c && c <= '9');
}

bool isWordChar(char c) {
    return isAlphaNum(c) || c == '_';
}

SourceFile loadFile(const std::filesystem::path& path) {
    assert(std::filesystem::status(path).type() == std::filesystem::file_type::regular);

    const size_t fileSize = std::filesystem::file_size(path);
    std::string source;
    source.resize(fileSize);

    std::ifstream f(path, std::ios_base::binary | std::ios_base::in);
    f.read(source.data(), source.size());
    const size_t bytesRead = f.gcount();

    if (bytesRead != source.size()) {
        source.resize(bytesRead);
    }

    constexpr size_t chunkSize = 4096;
    while (!f.eof()) {
        source.resize(source.size() + chunkSize);
        f.read(source.data() + source.size() - chunkSize, chunkSize);
        const size_t bytesRead = f.gcount();
        if (bytesRead != chunkSize) {
            source.resize(source.size() - chunkSize + bytesRead);
        }
    }

    return SourceFile {
        path,
        source
    };
}


TokenisedFile tokenise(SourceFile&& sourceFile) {
    std::string_view remaining(sourceFile.content.begin(), sourceFile.content.end());

    std::vector<Token> tokens;
    std::vector<Error> errors;

    bool previousTokenWasUnrecognised = false;
    auto curPos = [&]{return sourceFile.content.size() - remaining.size();};
    while(!remaining.empty()) {
        bool thisTokenWasUnrecognised = false;

        if (isWhiteSpace(remaining.front())) {
            remaining.remove_prefix(1);
            continue;  
        }

        // String Literal
        if (remaining.front() == '"' || remaining.front() == '\'') {
            size_t closingPos = remaining.find(remaining.front(), 1);
            size_t newlinePos = remaining.find('\n');

            if (closingPos == std::string_view::npos) {
                errors.push_back(Error{"No matching \" or \'", getLocation(sourceFile, curPos())});
                remaining = "";
            }

            else if (newlinePos < closingPos) {
                errors.push_back(Error{"No matching \" or \' before end of line", getLocation(sourceFile, curPos())});
                remaining = "";
            }

            else {
                std::string_view stringLit(remaining.data(), closingPos + 1);
                tokens.emplace_back(curPos(), TokenV::StringLiteral{stringLit});
                remaining.remove_prefix(closingPos + 1);
            }
        }

        // Word
        else if (isAlpha(remaining.front())) {
            size_t wordSize = 0;
            while (wordSize < remaining.size() && isWordChar(remaining[wordSize])) {
                wordSize++;
            }

            tokens.emplace_back(curPos(), TokenV::Word{std::string_view(remaining.data(), wordSize)});
            remaining.remove_prefix(wordSize);
        }

        // Comment
        else if (remaining.starts_with("//")) {
            size_t newlinePos = remaining.find('\n', 2);
            if (newlinePos == std::string_view::npos) {
                newlinePos = remaining.size();
            }

            tokens.emplace_back(curPos(), TokenV::Comment{});
            remaining.remove_prefix(newlinePos);
        }

        // Open Curly Brace
        else if (remaining.front() == '{') {
            tokens.emplace_back(curPos(), TokenV::CurlyBracket{.open=true});
            remaining.remove_prefix(1);
        }

        // Close Curly Brace
        else if (remaining.front() == '}') {
            tokens.emplace_back(curPos(), TokenV::CurlyBracket{.open=false});
            remaining.remove_prefix(1);
        }

        // Open Square Brace
        else if (remaining.front() == '[') {
            tokens.emplace_back(curPos(), TokenV::SquareBracket{.open=true});
            remaining.remove_prefix(1);
        }

        // Close Square Brace
        else if (remaining.front() == ']') {
            tokens.emplace_back(curPos(), TokenV::SquareBracket{.open=false});
            remaining.remove_prefix(1);
        }

        // Thin Arrow
        else if (remaining.starts_with("->")) {
            tokens.emplace_back(curPos(), TokenV::ThinArrow{});
            remaining.remove_prefix(2);
        }

        // Comma
        else if (remaining.front() == ',') {
            tokens.emplace_back(curPos(), TokenV::Comma{});
            remaining.remove_prefix(1);
        }

        // Double Colon
        else if (remaining.starts_with("::")) {
            tokens.emplace_back(curPos(), TokenV::DoubleColon{});
            remaining.remove_prefix(2);
        }

        else {
            if (!previousTokenWasUnrecognised) {
                errors.push_back(Error{"Unrecognised Token" , getLocation(sourceFile, curPos())});
            }
            thisTokenWasUnrecognised = true;

            const size_t endOfLine = remaining.find('\n');
            remaining.remove_prefix(endOfLine + 1);
        }
        previousTokenWasUnrecognised = thisTokenWasUnrecognised;
    }

    return TokenisedFile{
        .sourceFile = std::move(sourceFile),
        .tokens = std::move(tokens),
        .errors = std::move(errors),
    };
}


class AstParser {
private:
    friend AstFile parseTokenisedFile(TokenisedFile&& tokenisedFile);

    AstParser(TokenisedFile&& tokenisedFile): 
        tokenisedFile(std::move(tokenisedFile)),
        tokens(this->tokenisedFile.tokens.begin(), this->tokenisedFile.tokens.end()),
        sourceFile(this->tokenisedFile.sourceFile)
    {
    }

    AstParser(const AstParser&) = delete;
    AstParser(AstParser&&) = delete;

    AstParser& operator=(const AstParser&) = delete;
    AstParser& operator=(AstParser&&) = delete;


    SourceLocation curLocation() {
        if (!tokens.empty()) {
            return getLocation(sourceFile, tokens.front().sourcePos);
        } else {
            return getLocation(sourceFile, sourceFile.content.size());
        }
    }

    void addError(std::string msg) {
        thisParseSuccessful = false;
        if (successfulParseSinceLastError) {
            successfulParseSinceLastError = false;
            errors.push_back(Error{std::move(msg), curLocation()});
        }
    }

    void popToken() {
        do {
            tokens = tokens.last(tokens.size() - 1);
        } while (!tokens.empty() && tokens.front().template is<TokenV::Comment>());
    }

    std::optional<std::vector<TokenV::Word>> parseNamePath() {
        std::vector<TokenV::Word> parts;
        
        if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
            addError("Expected word");
            return std::nullopt;
        }

        parts.push_back(tokens.front().template get<TokenV::Word>());
        popToken();

        while (!tokens.empty() && tokens.front().template is<TokenV::DoubleColon>()) {
            popToken();

            if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
                addError("Expected word");
                return std::nullopt;
            }

            parts.push_back(tokens.front().template get<TokenV::Word>());
            popToken();
        }
        return parts;
    }

    std::optional<std::unique_ptr<AstNode>> parseTypeName() {
        assert(!tokens.empty());
        auto sourcePos = tokens.front().sourcePos;

        auto parts = parseNamePath();
        if (!parts) {
            return std::nullopt;
        }

        std::optional<std::vector<AstNode>> templateParams;
        // optional template parameters
        if (!tokens.empty() && tokens.front().template is<TokenV::SquareBracket>() && tokens.front().template get<TokenV::SquareBracket>().open) {
            templateParams = std::vector<AstNode>();
            popToken();

            auto frontIsClosingBrace = [&]{
                return tokens.front().template is<TokenV::SquareBracket>() && !tokens.front().template get<TokenV::SquareBracket>().open;
            };


            while (!tokens.empty() && !frontIsClosingBrace()) {
                auto param = parseTypeName();
                if (!param) {
                    return std::nullopt;
                }

                templateParams->push_back(std::move(**param));

                if (tokens.empty() || (!tokens.front().template is<TokenV::Comma>() && !frontIsClosingBrace())) {
                    addError("Expected , or ]");
                    return std::nullopt;
                }

                if (tokens.front().template is<TokenV::Comma>()) {
                    popToken();
                }
            }

            if (tokens.empty()) {
                addError("Expected ]");
                return std::nullopt;
            }

            popToken();
        }

        return std::make_unique<AstNode>(sourcePos, AstNodeV::TypeName{
            .nameParts = std::move(*parts),
            .templateParams = std::move(templateParams)
        });
    }

    std::optional<std::vector<AstNode>> parseTemplateParams() {
        // pop "["
        popToken();

        auto frontIsClosingBrace = [&]{
            return tokens.front().template is<TokenV::SquareBracket>() && !tokens.front().template get<TokenV::SquareBracket>().open;
        };

        std::vector<AstNode> rtn;

        while (!tokens.empty() && !frontIsClosingBrace()) {
            if (!tokens.front().template is<TokenV::Word>()) {
                addError("Expecting word");
                return std::nullopt;
            }

            rtn.push_back(AstNode(tokens.front().sourcePos, AstNodeV::TemplateParam{tokens.front().template get<TokenV::Word>()}));
            popToken();

            if (tokens.empty() || (!tokens.front().template is<TokenV::Comma>() && !frontIsClosingBrace())) {
                addError("Expected , or ]");
                return std::nullopt;
            }

            if (tokens.front().template is<TokenV::Comma>()) {
                popToken();
            }
        }

        if (tokens.empty()) {
            addError("Expected ]");
            return std::nullopt;
        }

        popToken();
        return rtn;
    }

    void parseItem() {
        AstNodeV::Item item;
        size_t sourcePos = tokens.front().sourcePos;

        // type
        item.type = tokens.front().template get<TokenV::Word>();
        popToken();

        // name
        if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
            addError("Expected word");
            return;
        }
        item.name = tokens.front().template get<TokenV::Word>();
        popToken();

        if (tokens.empty()) {
            addError("Expected word, -> or [");
            return;
        }

        // optional template parameters
        if (tokens.front().template is<TokenV::SquareBracket>() && tokens.front().template get<TokenV::SquareBracket>().open) {
            auto templateParams = parseTemplateParams();
            if (!templateParams) {
                return;
            }

            item.templateParams = std::move(*templateParams);

            if (tokens.empty()) {
                addError("Expected word or ->");
                return;
            }
        }


        // optional response type
        if (tokens.front().template is<TokenV::ThinArrow>()) {
            popToken();
            if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
                addError("Expected word");
                return;
            }

            auto responseTypeName = parseTypeName();
            if (!responseTypeName) {
                return;
            }
            item.responseType = std::move(*responseTypeName);
        }

        // opening brace
        if (tokens.empty() || !tokens.front().template is<TokenV::CurlyBracket>()) {
            addError("Expected {");
            return;
        }

        popToken();

        while (!tokens.empty() && !tokens.front().template is<TokenV::CurlyBracket>()) {
            AstNodeV::ItemMember member;
            if (!tokens.front().template is<TokenV::Word>()) {
                addError("Expected member type");
                return;
            }

            auto memberTypeName = parseTypeName();
            if (!memberTypeName) {
                return;
            }

            member.type = std::move(*memberTypeName);

            if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
                addError("Expected member name");
                return;
            }
            member.name = tokens.front().template get<TokenV::Word>();
            item.members.push_back(AstNode(tokens.front().sourcePos, std::move(member)));
            popToken();
        }

        if (tokens.empty() || tokens.front().template get<TokenV::CurlyBracket>().open) {
            addError("Expected }");
            return;
        }

        ast.items.push_back(AstNode(sourcePos, std::move(item)));
        popToken();
    }

    void parseNamespace() {
        popToken();
        if (tokens.front().template is<TokenV::Word>()) {
            ast.items.push_back(AstNode(
                tokens.front().sourcePos,
                AstNodeV::NamespaceSpec{tokens.front().template get<TokenV::Word>()}
            ));
            popToken();
        } else {
            addError("Expected word");
        }
    }

    void parseImport() {
        auto import = AstNodeV::TypeImport{};
        size_t sourcePos = tokens.front().sourcePos;

        popToken();
        if (tokens.empty()) {
            addError("Expected type name");
            return;
        }

        auto parts = parseNamePath();
        if (!parts) {
            return;
        }

        import.nameParts = std::move(*parts);

        //optional template params
        if (!tokens.empty() && tokens.front().template is<TokenV::SquareBracket>() && tokens.front().template get<TokenV::SquareBracket>().open) {
            auto templateParams = parseTemplateParams();
            if (!templateParams) {
                return;
            }

            import.templateParams = std::move(*templateParams);
        }

        ast.items.push_back(AstNode(sourcePos, std::move(import)));
    }

    void parse() {
        while (!tokens.empty()) {
            if (tokens.front().template is<TokenV::Comment>()) {
                popToken();
                continue;                
            }

            thisParseSuccessful = true;

            if (tokens.front().template is<TokenV::Word>()) {
                auto token = tokens.front().template get<TokenV::Word>();
                if (token.s == "namespace") {
                    parseNamespace();
                } else if (token.s == "import") {
                    parseImport();
                } else {
                    parseItem();
                }
            } else {
                addError("Unexpected token");
                popToken();
            }

            if (thisParseSuccessful) {
                successfulParseSinceLastError = true;
            }
        }
    }

    TokenisedFile tokenisedFile;
    AstNodeV::File ast;
    std::vector<Error> errors;
    std::span<Token> tokens;
    SourceFile& sourceFile;
    bool successfulParseSinceLastError = true;
    bool thisParseSuccessful = true;
};

AstFile parseTokenisedFile(TokenisedFile&& tokenisedFile) {
    AstParser parser(std::move(tokenisedFile));
    parser.parse();
    return AstFile{
        .sourceFile = std::move(parser.tokenisedFile.sourceFile),
        .ast = std::move(parser.ast),
        .errors = std::move(parser.errors)
    };
}


AstFile parse(const std::filesystem::path& path) {
    return parse(loadFile(path));
}

AstFile parse(SourceFile source) {
    TokenisedFile tokenisedFile = tokenise(std::move(source));

    if (!tokenisedFile.errors.empty()) {
        return AstFile{
            .sourceFile = std::move(tokenisedFile.sourceFile),
            .ast = AstNodeV::File{},
            .errors = std::move(tokenisedFile.errors)
        };
    }

    return parseTokenisedFile(std::move(tokenisedFile));
}

}