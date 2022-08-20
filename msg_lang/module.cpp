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

namespace pt::msg_lang {
std::string ItemName::joinPath(const std::string& sep) const {
    std::string rtn;
    bool addSep = false;
    for (const auto& s: path) {
        if (addSep) {
            rtn.append(sep);
        }
        addSep = true;
        rtn.append(s);
    }
    return rtn;
}
}

namespace pt::msg_lang::module {


size_t ItemNameHash::operator()(const ItemName& n) const {
    size_t hash = 0;
    for (const auto& name: n.path) {
        hash ^= std::hash<std::string>{}(name);
    }
    return hash;
}

namespace {

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
        checkForCyclicTypeDependencies();
        checkNumTemplateArguments();
        checkMessageTypes();
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
        auto wrap = [&](auto inner){
            return DataType(pos, f, std::move(inner));
        };

        if (typeName.nameParts.size() == 1) {
            std::string_view s = typeName.nameParts[0].s;
            if (s == "int") {
                return wrap(BuiltinType::Int);
            }
            else if (s == "uint") {
                return wrap(BuiltinType::UInt);
            }
            else if (s == "void") {
                return wrap(BuiltinType::Void);
            }
            else if (s == "bool") {
                return wrap(BuiltinType::Bool);
            }
            else if (s == "byte") {
                return wrap(BuiltinType::Byte);
            }
            else if (s == "usize") {
                return wrap(BuiltinType::USize);
            }
            else if (s == "float") {
                return wrap(BuiltinType::Float);
            }
            else if (s == "double") {
                return wrap(BuiltinType::Double);
            }
            else if (s == "str") {
                return wrap(BuiltinType::String);
            }
            else if (s == "list"){
                return wrap(BuiltinType::List);
            }
            else if (s == "option") {
                return wrap(BuiltinType::Option);
            }
            else {
                // check the template params first, then the messages in messageItems
                if (m.templateParams) {
                    for (const auto& p: *m.templateParams) {
                        if (p.name == s) {
                            return wrap(p);
                        }
                    }
                }

                // then imported types and defined messages, the order here doesn't
                // matter since conflicts between imported types and defined messages
                // aren't allowed
                auto imported = mod.getImportedType(ItemName{s});
                if (imported) {
                    return wrap(**imported);
                }


                auto it = messageItems.find(ItemName{std::string(s)});
                if (it != messageItems.end()) {
                    return wrap(it->second.handle);
                }
                
                addError(Error{"Unknown type", getLocation(f, pos)});
                return wrap(ErrorDataType{});
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
                        return wrap(TemplateMemberType{p, std::move(parts)});
                    }
                }
            }

            std::vector<std::string> nameParts;
            for (const auto& p: typeName.nameParts) {
                nameParts.emplace_back(p.s);
            }
            auto imported = mod.getImportedType(ItemName(std::move(nameParts)));
            if (imported) {
                return wrap(**imported);
            }

            addError(Error{"No such template parameter, member types are only supported for template parameters and imported types", getLocation(f, pos)});
            return wrap(ErrorDataType{});
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

            return DataType(pos, f, module::TemplateInstance{
                .template_ = Box(parseDataTypeName(node, m, f, pos)),
                .args = std::move(args)
            });
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

                    MessageType messageType = [&]{
                        if (item.type.s == "event") {
                            return MessageType::Event;
                        } else if (item.type.s == "request") {
                            return MessageType::Request;
                        } else if (item.type.s == "data") {
                            return MessageType::Data;
                        } else if (item.type.s == "union") {
                            return MessageType::Union;
                        } else {
                            addError(Error{
                                .message = "Invalid message type",
                                .location = getLocation(file.sourceFile, node.sourcePos),
                            });
                            return MessageType::ErrorMessageType;
                        }
                    }();

                    auto handle = mod.addMessage(Message{node.sourcePos, &file.sourceFile, name, messageType, {}, std::nullopt});
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

    void checkForCyclicTypeDependencies() {
        auto cycle = mod.findTopologicalCycle();

        if (!cycle) {
            return;
        }


        assert(!cycle->empty());

        std::string cycleString;
        bool addArrow = false;
        for (const MessageHandle& h: *cycle) {
            if (addArrow) {
                cycleString.append(" -> ");
            }
            addArrow = true;
            cycleString.append(mod.getMessage(h).name.joinPath("::"));
        }

        const auto& message = mod.getMessage(cycle->front());
        addError(Error{
            .message = "Message cycle: " + cycleString,
            .location = getLocation(*message.sourceFile, message.sourcePos)
        });
    }

    std::optional<size_t> expectedNumTemplateArgs(const DataType& dataType) {
        return dataType.visit(
            [](const BuiltinType& b) -> std::optional<size_t> {
                switch (b) {
                    case BuiltinType::Double:
                    case BuiltinType::Float:
                    case BuiltinType::Int:
                    case BuiltinType::UInt:
                    case BuiltinType::USize:
                    case BuiltinType::String:
                    case BuiltinType::Bool:
                    case BuiltinType::Void:
                    case BuiltinType::Byte:
                        return std::nullopt;
                    case BuiltinType::List:
                    case BuiltinType::Option:
                        return 1;
                }
                return std::nullopt;
            },
            [&](const MessageHandle& h) -> std::optional<size_t> {
                const auto& templateParams = mod.getMessage(h).templateParams;
                if (templateParams) {
                    return templateParams->size();
                } else {
                    return std::nullopt;
                }
            },
            [](const ImportedType& i) -> std::optional<size_t> {
                if (i.templateParams) {
                    return i.templateParams->size();
                } else {
                    return std::nullopt;
                }
            },
            [&](const TemplateInstance& instance) -> std::optional<size_t> {
                assert(!instance.template_->template is<TemplateInstance>());
                return expectedNumTemplateArgs(*instance.template_);
            },
            [](const TemplateParameter&) -> std::optional<size_t> {return std::nullopt;},
            [](const ErrorDataType&) -> std::optional<size_t> {return std::nullopt;},
            [](const TemplateMemberType&) -> std::optional<size_t> {return std::nullopt;}
        );
    }

    std::optional<size_t> actualNumTemplateArgs(const DataType& dataType) {
        return dataType.visit(
            [](const BuiltinType& b) -> std::optional<size_t> {return std::nullopt;},
            [](const MessageHandle& h) -> std::optional<size_t> {return std::nullopt;},
            [](const TemplateParameter&) -> std::optional<size_t> {return std::nullopt;},
            [](const ErrorDataType&) -> std::optional<size_t> {return std::nullopt;},
            [](const TemplateMemberType&) -> std::optional<size_t> {return std::nullopt;},
            [](const ImportedType& i) -> std::optional<size_t> {return std::nullopt;},
            [](const TemplateInstance& instance) -> std::optional<size_t> {return instance.args.size();}
        );
    }

    void checkNumTemplateArgumentsForDataType(const DataType& dataType) {
        auto expected = expectedNumTemplateArgs(dataType);
        auto actual = actualNumTemplateArgs(dataType);

        if (actual != expected) {
            const std::string expectedMessage = [&]() -> std::string {
                if (expected) {
                    return "Expected " + std::to_string(*expected) + " template arguments.";
                } else {
                    return "Is not a template.";
                }
            }();

            const std::string actualMessage = [&]() -> std::string {
                if (actual) {
                    return " Got " + std::to_string(*actual) + " template arugments.";
                } else {
                    return " Got no template arguments";
                }
            }();

            addError(Error{
                .message = expectedMessage + actualMessage,
                .location = dataType.sourceLocation()
            });
        }  
    }

    void checkNumTemplateArguments() {
        for (const auto& item: messageItems) {
            const auto& message = mod.getMessage(item.second.handle);

            if (message.expectedResponse) {
                checkNumTemplateArgumentsForDataType(*message.expectedResponse);
            }

            for (const auto& member: message.members) {
                checkNumTemplateArgumentsForDataType(member.type);
            }
        }
    }

    void checkMessageTypes() {
        for (const auto& item: messageItems) {
            const auto& message = mod.getMessage(item.second.handle);

            switch (message.type) {
                case MessageType::Data:
                case MessageType::Union:
                case MessageType::Event: {
                    if (message.expectedResponse) {
                        addError(Error{
                            .message = "Message type does not allow a response",
                            .location = getLocation(*message.sourceFile, message.sourcePos),
                        });
                    }
                    break;
                }

                case MessageType::Request: {
                    if (!message.expectedResponse) {
                        addError({
                            .message = "Message type requires a response type",
                            .location = getLocation(*message.sourceFile, message.sourcePos),
                        });
                    }
                    break;
                }

                case MessageType::ErrorMessageType: {
                    break;
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

std::vector<MessageHandle> Module::dependencies(MessageHandle handle) const {
    std::unordered_set<size_t> deps;
    for (const auto& member: getMessage(handle).members) {
        forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
            deps.insert(h.idx);
        });
    }

    std::vector<MessageHandle> rtn;
    for (size_t dep: deps) {
        rtn.push_back(MessageHandle(dep));
    }
    return rtn;
}

std::optional<std::vector<MessageHandle>> Module::findTopologicalCycle() const {
    std::vector<bool> hasDep;
    hasDep.resize(messages_.size());

    for (const auto& message: messages_) {
        for (const auto& member: message.members) {
            forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
                hasDep[h.idx] = true;
            });
        }
    }

    std::vector<size_t> roots;
    for (size_t i = 0; i < hasDep.size(); i++) {
        if (!hasDep[i]) {
            roots.push_back(i);
        }
    }

    std::vector<bool> inStack;
    inStack.resize(messages_.size());

    std::vector<bool> visited;
    visited.resize(messages_.size());
    auto checkFrom = [&](size_t root) -> std::optional<std::vector<MessageHandle>> {
        std::vector<MessageHandle> toCheck;
        std::vector<std::pair<MessageHandle, size_t>> stack;

        toCheck.push_back(MessageHandle{root});

        while(!toCheck.empty()) {
            MessageHandle thisNode = toCheck.back();
            visited[thisNode.idx] = true;
            toCheck.pop_back();
            if (!stack.empty()) {
                stack.back().second--;
            }

            if (!inStack[thisNode.idx]) {
                std::vector<MessageHandle> deps = dependencies(thisNode);

                if (!deps.empty()) {
                    stack.emplace_back(thisNode, deps.size());
                    inStack[thisNode.idx] = true;
                    toCheck.insert(toCheck.end(), deps.begin(), deps.end());
                }
            } else {
                std::vector<MessageHandle> cycle;
                bool foundStart = false;
                for (const auto& frame: stack) {
                    if (frame.first == thisNode) {
                        foundStart = true;
                    }

                    if (foundStart) {
                        cycle.push_back(frame.first);
                    }
                }
                cycle.push_back(thisNode);
                return cycle;
            }

            while (!stack.empty() && stack.back().second == 0) {
                inStack[stack.back().first.idx] = false;
                stack.pop_back();
            }
        }
        assert(stack.empty());
        return std::nullopt;
    };

    for (size_t root: roots) {
        auto res = checkFrom(root);
        if (res) {
            return res;
        }
    }

    for (size_t root = 0; root < messages_.size(); root++) {
        if (!visited[root]) {
            auto res = checkFrom(root);
            if (res) {
                return res;
            }
        }
    }

    return std::nullopt;
}

std::vector<MessageHandle> Module::topologicalOrder() const {
    std::vector<MessageHandle> rtn;

    // edge from set member to index (from dependent to dependency)
    std::vector<std::unordered_set<size_t>> edges;
    edges.resize(messages_.size());

    size_t numEdgesRemaining = 0;
    for (size_t i = 0; i < messages_.size(); i++) {
        const auto& message = messages_[i];
        for (const auto& member: message.members) {
            forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
                auto r = edges[h.idx].insert(i);
                if (r.second) {
                    numEdgesRemaining++;
                }
            });
        }
    }

    std::vector<size_t> roots;
    for (size_t i = 0; i < edges.size(); i++) {
        if (edges[i].empty()) {
            roots.push_back(i);
        }
    }

    while (!roots.empty()) {
        size_t root = roots.back();
        roots.pop_back();
        rtn.push_back(MessageHandle{root});

        const auto& message = messages_[root];
        for (const auto& member: message.members) {
            forEachTypeDependencies(member.type, [&](const module::MessageHandle& h) {
                size_t numRemoved = edges[h.idx].erase(root);
                numEdgesRemaining -= numRemoved;

                if (numRemoved > 0 && edges[h.idx].empty()) {
                    roots.push_back(h.idx);
                }
            });
        }
    }

    assert(numEdgesRemaining == 0);

    return rtn;
}

}