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

    if (!f.eof()) {
        constexpr size_t chunkSize = 4096;
        while (!f.eof()) {
            source.resize(source.size() + chunkSize);
            f.read(source.data() + source.size() - chunkSize, chunkSize);
            const size_t bytesRead = f.gcount();
            if (bytesRead != chunkSize) {
                source.resize(source.size() - chunkSize + bytesRead);
            }
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

        while (isWhiteSpace(remaining.front())) {
            remaining.remove_prefix(1);
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

        // Open Brace
        else if (remaining.front() == '{') {
            tokens.emplace_back(curPos(), TokenV::CurlyBracket{.open=true});
            remaining.remove_prefix(1);
        }

        // Close Brace
        else if (remaining.front() == '}') {
            tokens.emplace_back(curPos(), TokenV::CurlyBracket{.open=false});
            remaining.remove_prefix(1);
        }

        else {
            if (!previousTokenWasUnrecognised) {
                errors.push_back(Error{"Unrecognised Token", getLocation(sourceFile, curPos())});
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
        tokens = tokens.last(tokens.size() - 1);
    }

    void parseItem() {
        AstNodeV::Item item;
        size_t sourcePos = tokens.front().sourcePos;
        item.type = tokens.front().template get<TokenV::Word>();
        popToken();

        if (tokens.empty() || !tokens.front().template is<TokenV::Word>()) {
            addError("Expected word");
            return;
        }

        item.name = tokens.front().template get<TokenV::Word>();
        popToken();

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
            member.type = tokens.front().template get<TokenV::Word>();
            popToken();

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

    void parse() {
        while (!tokens.empty()) {
            while (tokens.front().template is<TokenV::Comment>()) {
                popToken();
            }

            thisParseSuccessful = true;

            if (tokens.front().template is<TokenV::Word>()) {
                parseItem();
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
    TokenisedFile tokenisedFile = tokenise(loadFile(path));

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