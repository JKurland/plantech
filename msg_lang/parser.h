#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <stdint.h>

#include "my_variant.h"
#include "error.h"

namespace pt::msg_lang {


namespace TokenV {
    struct Word {std::string_view s;};
    struct CurlyBracket {bool open;};
    struct Comment {};
    struct StringLiteral {std::string_view s;};
}

class Token: public MyVariant<TokenV::Word, TokenV::CurlyBracket, TokenV::Comment, TokenV::StringLiteral> {
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
        std::vector<AstNode> members;
    };

    struct ItemMember {
        TokenV::Word type;
        TokenV::Word name;
    };
}

class AstNode: public MyVariant<AstNodeV::File, AstNodeV::Item, AstNodeV::ItemMember> {
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
}
