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
    std::weak_ordering operator<=>(const ItemName&) const = default;
};

namespace module {

enum class BuiltinType {
    Int,
    Float,
    String,
};


class MessageHandle {
    MessageHandle(size_t idx): idx(idx) {}
    size_t idx;
    friend class Module;
};

using DataType = MyVariant<BuiltinType, MessageHandle>;

struct MessageMember {
    DataType type;
    std::string name;
};


struct Message {
    ItemName name;
    std::vector<MessageMember> members;
    std::optional<DataType> expectedResponse;
};

class Module {
public:
    MessageHandle addMessage(Message message);
    Message& getMessage(MessageHandle handle);
    const Message& getMessage(MessageHandle handle) const;

    const std::vector<Message>& messages() const;
private:

    std::vector<Message> messages_;
};

Module compile(const std::vector<AstFile>& files);

}
}