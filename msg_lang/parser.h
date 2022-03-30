#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <optional>
#include <stdint.h>

#include "my_variant.h"
#include "error.h"

namespace pt::msg_lang {


namespace TokenV {
    struct Word {std::string_view s;};
    struct CurlyBracket {bool open;};
    struct Comment {};
    struct StringLiteral {std::string_view s;};
    struct ThinArrow {};
    struct SquareBracket {bool open;};
    struct Comma {};
}

class Token: public MyVariant<TokenV::Word, TokenV::CurlyBracket, TokenV::Comment, TokenV::StringLiteral, TokenV::ThinArrow, TokenV::SquareBracket, TokenV::Comma> {
public:
    template<typename...Ts>
    Token(size_t sourcePos, Ts&&...args): MyVariant(std::forward<Ts>(args)...), sourcePos(sourcePos) {}

    size_t sourcePos;
};

class AstNode;
namespace AstNodeV {
    struct File {
        std::vector<AstNode> items;
    };

    struct Item {
        TokenV::Word type;
        TokenV::Word name;
        std::optional<std::vector<AstNode>> templateParams;
        std::optional<TokenV::Word> responseType;
        std::vector<AstNode> members;
    };

    struct ItemMember {
        TokenV::Word type;
        TokenV::Word name;
    };

    struct NamespaceSpec {
        TokenV::Word ns;
    };

    struct TemplateParam {
        TokenV::Word name;
    };
}

class AstNode: public MyVariant<
    AstNodeV::File,
    AstNodeV::Item,
    AstNodeV::ItemMember,
    AstNodeV::NamespaceSpec,
    AstNodeV::TemplateParam
> {
public:
    template<typename...Ts>
    AstNode(size_t sourcePos, Ts&&...args): MyVariant(std::forward<Ts>(args)...), sourcePos(sourcePos) {}

    size_t sourcePos;
};


struct AstFile {
    SourceFile sourceFile;
    AstNodeV::File ast;
    std::vector<Error> errors;
};

// if AstFile.errors is not empty on return then the ast does not necessarily contain anything useful
// although it might be worth processing it further to find more errors
AstFile parse(const std::filesystem::path& path);
AstFile parse(SourceFile source);
}
