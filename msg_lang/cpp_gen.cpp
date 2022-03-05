#include "cpp_gen.h"

#include "module.h"
#include <iostream>

namespace pt::msg_lang {


std::string genCpp(const module::Module& module) {
    std::string source;

    auto dumpDataType = [&](const module::DataType& dataType) -> std::string{
        if (dataType.template is<module::BuiltinType>()) {
            switch (dataType.template get<module::BuiltinType>()) {
                case module::BuiltinType::Int: {
                    return "std::int32_t";
                }
                case module::BuiltinType::Float: {
                    return "float";
                }
                case module::BuiltinType::String: {
                    return "std::string";
                }
            }
        }
    
        assert(dataType.template is<module::MessageHandle>());
        return module.getMessage(dataType.template get<module::MessageHandle>()).name.name;
    };

    source.append("#include <string>\n");
    source.append("#include <cstdint>\n");

    // for now just forward declare everything then define everything
    for (const auto& message: module.messages()) {
        source.append("struct ");
        source.append(message.name.name);
        source.push_back(';');
        source.push_back('\n');
    }

    // now define everything
    for (const auto& message: module.messages()) {
        source.append("struct ");
        source.append(message.name.name);
        source.append(" {\n");
        for (const auto& member: message.members) {
            source.append("    ");
            source.append(dumpDataType(member.type));
            source.push_back(' ');
            source.append(member.name);
            source.append(";\n");
        }
        source.append("};\n");
    }

    return source;
}

}