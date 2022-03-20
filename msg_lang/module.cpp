#include "module.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <cassert>
#include <iostream>
#include <variant>

#include "parser.h"

namespace pt::msg_lang::module {

namespace {

struct ItemNameHash {
    size_t operator()(const ItemName& n) const {
        return std::hash<std::string>{}(n.name);
    }
};

struct ItemFromFile {
    AstNode itemNode;
    const AstFile* file;
    MessageHandle handle;
};

class ModuleBuilder {
public:
    ModuleBuilder(const std::vector<AstFile>& files): files(files) {}

    Module build() {
        findMessageItems();
        fillInMessageMembers();
        return mod;
    }

private:
    const std::vector<AstFile>& files;
    std::unordered_map<ItemName, ItemFromFile, ItemNameHash> messageItems;
    Module mod;

    DataType parseDataType(std::string_view s) {
        if (s == "int") {
            return BuiltinType::Int;
        }
        else if (s == "float") {
            return BuiltinType::Float;
        }
        else if (s == "str") {
            return BuiltinType::String;
        }
        else {
            auto it = messageItems.find(ItemName{std::string(s)});
            assert(it != messageItems.end());
            return DataType{it->second.handle};
        }
    }

    void findMessageItems() {
        for (const AstFile& file: files) {
            for (const AstNode& node: file.ast.items) {
                if (node.template is<AstNodeV::Item>()) {
                    const auto& item = node.template get<AstNodeV::Item>();
                    ItemName name{std::string{item.name.s}};

                    auto handle = mod.addMessage(Message{name, {}, std::nullopt});
                    bool inserted = messageItems.emplace(name, ItemFromFile{node, &file, handle}).second;
                    assert(inserted);
                    (void)inserted;
                }
            }
        }
    }

    void fillInMessageMembers() {
        for (auto& item: messageItems) {
            auto& itemFromFile = item.second;
            assert(itemFromFile.itemNode.template is<AstNodeV::Item>());
            auto& message = mod.getMessage(itemFromFile.handle);
            for (const auto& memberNode: itemFromFile.itemNode.template get<AstNodeV::Item>().members) {
                assert(memberNode.template is<AstNodeV::ItemMember>());
                auto& member = memberNode.template get<AstNodeV::ItemMember>();

                message.members.push_back(MessageMember{parseDataType(member.type.s), std::string(member.name.s)});
            }
        }
    }
};

}

Module compile(const std::vector<AstFile>& files) {
    return ModuleBuilder(files).build();
}

MessageHandle Module::addMessage(Message message) {
    auto handle = MessageHandle{messages_.size()};
    messages_.push_back(std::move(message));
    return handle;
}

Message& Module::getMessage(MessageHandle handle) {
    return messages_[handle.idx];
}

const Message& Module::getMessage(MessageHandle handle) const {
    return messages_[handle.idx];
}

const std::vector<Message>& Module::messages() const {
    return messages_;
}

bool Module::hasMessage(const ItemName& name) const {
    for (const auto& m: messages_) {
        if (m.name == name) {
            return true;
        }
    }
    return false;
}

std::optional<const Message*> Module::messageByName(const ItemName& name) const {
    for (const auto& m: messages_) {
        if (m.name == name) {
            return &m;
        }
    }

    return std::nullopt;
}

}