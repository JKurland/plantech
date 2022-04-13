#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <compare>
#include <unordered_map>

#include "my_variant.h"
#include "box.h"
#include "error.h"

namespace pt::msg_lang {
class AstFile;

struct ItemName {
    template<typename T1, typename...Ts, typename = std::enable_if_t<std::is_constructible_v<std::string, std::decay_t<T1>>>>
    explicit ItemName(T1&& first, Ts&&...args) {
        // must have at least 1 part
        path.reserve(sizeof...(Ts) + 1);
        path.emplace_back(std::forward<T1>(first));
        (path.emplace_back(std::forward<Ts>(args)), ...);
    }

    explicit ItemName(std::vector<std::string> path): path(std::move(path)) {}

    ItemName(const ItemName&) = default;
    ItemName(ItemName&&) = default;

    ItemName& operator=(const ItemName&) = default;
    ItemName& operator=(ItemName&&) = default;

    ~ItemName() = default;

    std::vector<std::string> path;
    auto operator<=>(const ItemName&) const = default;
};

namespace module {
struct ItemNameHash {
    size_t operator()(const ItemName& n) const;
};

class DataType;

enum class BuiltinType {
    Int,
    Float,
    String,
};


class MessageHandle {
public:
    auto operator<=>(const MessageHandle&) const = default;

private:
    MessageHandle(size_t idx): idx(idx) {}
    size_t idx;
    friend class Module;
};

struct TemplateParameter {
    std::string name;

    auto operator<=>(const TemplateParameter&) const = default;
};

// a type that is produced when there is some error in the source
struct ErrorDataType {
    auto operator<=>(const ErrorDataType&) const = default;
};


// A member type of a template parameter, for example, T::iterator_type
struct TemplateMemberType {
    TemplateParameter param;
    std::vector<std::string> path;

    auto operator<=>(const TemplateMemberType&) const = default;
};

struct ImportedType {
    ItemName name;
    std::optional<std::vector<TemplateParameter>> templateParams;

    auto operator<=>(const ImportedType&) const = default;
    bool operator==(const ImportedType&) const = default;
};

struct TemplateInstance {
    Box<DataType> template_;
    std::vector<DataType> args;

    auto operator<=>(const TemplateInstance&) const = default;
};

class DataType: public MyVariant<BuiltinType, MessageHandle, TemplateParameter, ErrorDataType, TemplateMemberType, ImportedType, TemplateInstance> {
    using MyVariant::MyVariant;
};


struct MessageMember {
    DataType type;
    std::string name;
};


struct Message {
    ItemName name;
    std::vector<MessageMember> members;
    std::optional<DataType> expectedResponse;
    std::optional<std::vector<TemplateParameter>> templateParams;
};


class Module {
public:
    MessageHandle addMessage(Message message);
    Message& getMessage(MessageHandle handle);
    const Message& getMessage(MessageHandle handle) const;

    const std::vector<Message>& messages() const;

    bool hasMessage(const ItemName& name) const;
    std::optional<const Message*> messageByName(const ItemName& name) const;

    std::optional<const ImportedType*> getImportedType(const ItemName& name) const;
    void addImportedType(ImportedType type);

    // the namespace the messages in this module should be generated in
    std::optional<std::string> withNamespace;
    std::vector<Error> errors;
private:
    std::unordered_map<ItemName, ImportedType, ItemNameHash> importedTypes_;
    std::vector<Message> messages_;
};

Module compile(const std::vector<AstFile>& files);

}
}