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

size_t ItemNameHash::operator()(const ItemName& n) const {
    size_t hash = 0;
    for (const auto& name: n.path) {
        hash ^= std::hash<std::string>{}(name);
    }
    return hash;
}

namespace {


struct ItemFromFile {
    const AstNode* itemNode;
    const AstFile* file;
    MessageHandle handle;
};

class ModuleBuilder {
public:
    ModuleBuilder(const std::vector<AstFile>& files): files(files) {}

    Module build() {
        findMessageItems();
        fillInTemplateParameters();
        processTypeImports();
        fillInMessageMembers();
        fillInMessageReponseTypes();
        processNamespaces();
        return mod;
    }

private:
    const std::vector<AstFile>& files;
    std::unordered_map<ItemName, ItemFromFile, ItemNameHash> messageItems;
    Module mod;

    void addError(Error e) {
        mod.errors.push_back(std::move(e));
    }

    DataType parseDataType(AstNode& node, const Message& m, const SourceFile& f, size_t pos) {
        assert(node.template is<AstNodeV::TypeName>());
        auto typeName = node.template get<AstNodeV::TypeName>();

        assert(!typeName.nameParts.empty());
        if (typeName.nameParts.size() == 1) {
            std::string_view s = typeName.nameParts[0].s;
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
                // check the template params first, then the messages in messageItems
                if (m.templateParams) {
                    for (const auto& p: *m.templateParams) {
                        if (p.name == s) {
                            return p;
                        }
                    }
                }

                auto it = messageItems.find(ItemName{std::string(s)});
                if (it == messageItems.end()) {
                    addError(Error{"Unknown type", getLocation(f, pos)});
                    return ErrorDataType{};
                }
                return DataType{it->second.handle};
            }
        } else {
            auto templateParamName = typeName.nameParts.front().s;
            if (m.templateParams) {
                for (const auto& p: *m.templateParams) {
                    if (p.name == templateParamName) {
                        std::vector<std::string> parts;
                        for (size_t i = 1; i < typeName.nameParts.size(); i++) {
                            parts.emplace_back(typeName.nameParts[i].s);
                        }
                        return TemplateMemberType{p, std::move(parts)};
                    }

                }
            }
            addError(Error{"No such template parameter, member types are only supported for template parameters", getLocation(f, pos)});
            return ErrorDataType{};
        }
    }

    void findMessageItems() {
        for (const AstFile& file: files) {
            for (const AstNode& node: file.ast.items) {
                if (node.template is<AstNodeV::Item>()) {
                    const auto& item = node.template get<AstNodeV::Item>();
                    ItemName name{item.name.s};

                    if (messageItems.contains(name)) {
                        addError(Error{"Redefinition of message", getLocation(file.sourceFile, node.sourcePos)});
                        continue;
                    }

                    auto handle = mod.addMessage(Message{name, {}, std::nullopt});
                    messageItems.emplace(name, ItemFromFile{&node, &file, handle}).second;
                }
            }
        }
    }

    void fillInTemplateParameters() {
        for (auto& item: messageItems) {
            auto& itemFromFile = item.second;
            assert(itemFromFile.itemNode->template is<AstNodeV::Item>());

            auto& itemNode = itemFromFile.itemNode->template get<AstNodeV::Item>();

            auto& message = mod.getMessage(itemFromFile.handle);

            if (itemNode.templateParams) {
                message.templateParams = std::vector<TemplateParameter>();
                for (const auto& p: *itemNode.templateParams) {
                    assert(p.template is<AstNodeV::TemplateParam>());
                    message.templateParams->push_back(
                        TemplateParameter{std::string(p.template get<AstNodeV::TemplateParam>().name.s)}
                    );
                }
            }
        }
    }

    void processTypeImports() {
        for (const AstFile& file: files) {
            for (const AstNode& node: file.ast.items) {
                if (node.template is<AstNodeV::TypeImport>()) {
                    const auto& typeImport = node.template get<AstNodeV::TypeImport>();

                    std::vector<std::string> path;
                    for (const auto& p: typeImport.nameParts) {
                        path.emplace_back(p.s);
                    }
                    ItemName name{path};

                    mod.addImportedType(name);
                }
            }
        }
    }

    void fillInMessageMembers() {
        for (auto& item: messageItems) {
            auto& itemFromFile = item.second;
            assert(itemFromFile.itemNode->template is<AstNodeV::Item>());
            auto& message = mod.getMessage(itemFromFile.handle);
            for (const auto& memberNode: itemFromFile.itemNode->template get<AstNodeV::Item>().members) {
                assert(memberNode.template is<AstNodeV::ItemMember>());
                auto& member = memberNode.template get<AstNodeV::ItemMember>();

                DataType dataType = parseDataType(*member.type, message, itemFromFile.file->sourceFile, memberNode.sourcePos);
                message.members.push_back(MessageMember{dataType, std::string(member.name.s)});
            }
        }
    }

    void fillInMessageReponseTypes() {
        for (auto& item: messageItems) {
            auto& itemFromFile = item.second;
            assert(itemFromFile.itemNode->template is<AstNodeV::Item>());
            auto& message = mod.getMessage(itemFromFile.handle);

            auto& itemNode = itemFromFile.itemNode->template get<AstNodeV::Item>();
            if (itemNode.responseType) {
                DataType dataType = parseDataType(**itemNode.responseType, message, itemFromFile.file->sourceFile, itemFromFile.itemNode->sourcePos);
                message.expectedResponse = dataType;
            }
        }
    }

    void processNamespaces() {
        for (const auto& file: files) {
            for (const AstNode& node: file.ast.items) {
                if (node.template is<AstNodeV::NamespaceSpec>()) {
                    if (!mod.withNamespace) {
                        mod.withNamespace = node.template get<AstNodeV::NamespaceSpec>().ns.s;
                    } else {
                        if (*mod.withNamespace != node.template get<AstNodeV::NamespaceSpec>().ns.s) {
                            addError(Error{
                                .message = "Conflicting namespace specifications",
                                .location = getLocation(file.sourceFile, node.sourcePos)
                            });
                        }
                    }
                }
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

bool Module::getImportedType(const ItemName& name) const {
    return importedTypes_.contains(name);
}

void Module::addImportedType(const ItemName& name) {
    importedTypes_.insert(name);
}

}