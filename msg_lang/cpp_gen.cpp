#include "cpp_gen.h"

#include "module.h"
#include <iostream>

namespace pt::msg_lang {


CppSource genCpp(const module::Module& module) {
    std::string header;

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

    header.append("#include <string>\n");
    header.append("#include <cstdint>\n");

    // for now just forward declare everything then define everything
    for (const auto& message: module.messages()) {
        header.append("struct ");
        header.append(message.name.name);
        header.push_back(';');
        header.push_back('\n');
    }

    // now define everything
    for (const auto& message: module.messages()) {
        header.append("struct ");
        header.append(message.name.name);
        header.append(" {\n");
        for (const auto& member: message.members) {
            header.append("    ");
            header.append(dumpDataType(member.type));
            header.push_back(' ');
            header.append(member.name);
            header.append(";\n");
        }
        header.append("};\n");
    }

    return CppSource{
        .header = header,
        .source = ""
    };
}

}