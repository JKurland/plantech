#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <compare>

#include "my_variant.h"
#include "error.h"

namespace pt::msg_lang {
class AstFile;

struct ItemName {
    std::string name;
    auto operator<=>(const ItemName&) const = default;
};

namespace module {

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

using DataType = MyVariant<BuiltinType, MessageHandle, TemplateParameter, ErrorDataType, TemplateMemberType>;


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

    // the namespace the messages in this module should be generated in
    std::optional<std::string> withNamespace;
    std::vector<Error> errors;
private:

    std::vector<Message> messages_;
};

Module compile(const std::vector<AstFile>& files);

}
}