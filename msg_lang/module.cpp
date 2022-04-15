#include "module.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
        checkForNameConflicts();
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

    DataType parseDataTypeName(const AstNode& node, const Message& m, const SourceFile& f, size_t pos) {
        assert(node.template is<AstNodeV::TypeName>());
        const auto& typeName = node.template get<AstNodeV::TypeName>();

        assert(!typeName.nameParts.empty());
        if (typeName.nameParts.size() == 1) {
            std::string_view s = typeName.nameParts[0].s;
            if (s == "int") {
                return BuiltinType::Int;
            }
            else if (s == "float") {
                return BuiltinType::Float;
            }
            else if (s == "double") {
                return BuiltinType::Double;
            }
            else if (s == "str") {
                return BuiltinType::String;
            }
            else if (s == "list"){
                return BuiltinType::List;
            }
            else if (s == "option") {
                return BuiltinType::Option;
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

                // then imported types and defined messages, the order here doesn't
                // matter since conflicts between imported types and defined messages
                // aren't allowed
                auto imported = mod.getImportedType(ItemName{s});
                if (imported) {
                    return DataType(**imported);
                }


                auto it = messageItems.find(ItemName{std::string(s)});
                if (it != messageItems.end()) {
                    return DataType{it->second.handle};
                }
                
                addError(Error{"Unknown type", getLocation(f, pos)});
                return ErrorDataType{};
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

            std::vector<std::string> nameParts;
            for (const auto& p: typeName.nameParts) {
                nameParts.emplace_back(p.s);
            }
            auto imported = mod.getImportedType(ItemName(std::move(nameParts)));
            if (imported) {
                return DataType(**imported);
            }

            addError(Error{"No such template parameter, member types are only supported for template parameters and imported types", getLocation(f, pos)});
            return ErrorDataType{};
        }
    }

    DataType parseDataType(const AstNode& node, const Message& m, const SourceFile& f, size_t pos) {
        assert(node.template is<AstNodeV::TypeName>());
        const auto& typeName = node.template get<AstNodeV::TypeName>();

        if (typeName.templateParams) {
            std::vector<DataType> args;

            for (const auto& arg: *typeName.templateParams) {
                args.push_back(parseDataType(arg, m, f, arg.sourcePos));
            }

            return module::TemplateInstance{
                .template_ = Box(parseDataTypeName(node, m, f, pos)),
                .args = std::move(args)
            };
        } else {
            return parseDataTypeName(node, m, f, pos);
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

                    

                    ImportedType type{
                        .name = ItemName{path},
                        .templateParams = std::nullopt
                    };

                    if (typeImport.templateParams) {
                        type.templateParams = std::vector<TemplateParameter>();
                        for (const auto& p: *typeImport.templateParams) {
                            assert(p.template is<AstNodeV::TemplateParam>());
                            type.templateParams->push_back(
                                TemplateParameter{std::string(p.template get<AstNodeV::TemplateParam>().name.s)}
                            );
                        }
                    }

                    mod.addImportedType(type);
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

    void checkForNameConflicts() {
        for (const auto& item: messageItems) {
            if (mod.getImportedType(item.first)) {
                addError(Error{
                    .message = "Imported type conflicts with message",
                    .location = getLocation(item.second.file->sourceFile, item.second.itemNode->sourcePos)
                });
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

std::optional<const ImportedType*> Module::getImportedType(const ItemName& name) const {
    auto it = importedTypes_.find(name);
    if (it == importedTypes_.end()) {
        return std::nullopt;
    }
    return &it->second;
}

void Module::addImportedType(ImportedType type) {
    auto name = type.name;
    importedTypes_.emplace(std::move(name), std::move(type));
}

template<typename F>
void forEachTypeDependencies(const DataType& type, F&& f) {
    type.visit(
        [&](const module::BuiltinType&) {},
        [&](const module::MessageHandle& handle) {f(handle);},
        [&](const module::TemplateParameter&) {},
        [&](const module::ErrorDataType&) {},
        [&](const module::TemplateMemberType&) {},
        [&](const module::ImportedType&) {},
        [&](const module::TemplateInstance& i) {
            // lists don't create a dependency
            if (i.template_->template is<BuiltinType>() && i.template_->template get<BuiltinType>() == BuiltinType::List) {
                return;
            }
            forEachTypeDependencies(*i.template_, f);
            for (const auto& t: i.args) {
                forEachTypeDependencies(t, f);
            }
        }
    );
}

std::vector<MessageHandle> Module::topologicalOrder() const {
    std::vector<MessageHandle> rtn;

    // edge from set member to index (from dependent to dependency)
    std::vector<std::unordered_set<size_t>> edges;
    edges.resize(messages_.size());

    std::unordered_set<size_t> roots;


    for (size_t i = 0; i < messages_.size(); i++) {
        roots.insert(i);
    }

    size_t numEdgesRemaining = 0;
    for (size_t i = 0; i < messages_.size(); i++) {
        const auto& message = messages_[i];
        for (const auto& member: message.members) {
            forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
                auto r = edges[h.idx].insert(i);
                roots.erase(h.idx);
                if (r.second) {
                    numEdgesRemaining++;
                }
            });
        }
    }

    while (!roots.empty()) {
        size_t root = *roots.begin();
        roots.erase(root);
        rtn.push_back(MessageHandle{root});

        const auto& message = messages_[root];
        for (const auto& member: message.members) {
            forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
                size_t numRemoved = edges[h.idx].erase(root);
                numEdgesRemaining -= numRemoved;

                if (numRemoved > 0 && edges[h.idx].empty()) {
                    roots.insert(h.idx);
                }
            });
        }
    }

    assert(numEdgesRemaining == 0);

    return rtn;
}

}