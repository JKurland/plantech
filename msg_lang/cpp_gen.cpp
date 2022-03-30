#include "cpp_gen.h"

#include "module.h"
#include <iostream>

namespace pt::msg_lang {

void appendTemplateString(const module::Message& message, std::string& s) {
    s.append("template<");
    bool skipComma = true;
    for (const auto& p: *message.templateParams) {
        if (!skipComma) {
            s.append(", ");
        }
        skipComma = false;

        s.append("typename ");
        s.append(p.name);
    }
    s.append(">\n");
}

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
        } else if (dataType.template is<module::ErrorDataType>()) {
            return "__ERROR__";
        } else if (dataType.template is<module::TemplateParameter>()) {
            return dataType.template get<module::TemplateParameter>().name;
        }

        assert(dataType.template is<module::MessageHandle>());
        return module.getMessage(dataType.template get<module::MessageHandle>()).name.name;
    };

    header.append("#include <string>\n");
    header.append("#include <cstdint>\n");

    if (module.withNamespace) {
        header.append("namespace ");
        header.append(*module.withNamespace);
        header.append(" {\n");
    }

    // for now just forward declare everything then define everything
    for (const auto& message: module.messages()) {
        if (message.templateParams) {
            appendTemplateString(message, header);
        }
        header.append("struct ");
        header.append(message.name.name);
        header.push_back(';');
        header.push_back('\n');
    }

    header.push_back('\n');

    // now define everything
    for (const auto& message: module.messages()) {
        if (message.templateParams) {
            appendTemplateString(message, header);
        }

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

        if (message.expectedResponse) {
            header.append("    using ResponseT = ");
            header.append(dumpDataType(*message.expectedResponse));
            header.append(";\n");
        }

        header.append("};\n");
    }

    if (module.withNamespace) {
        header.push_back('}');
    }

    return CppSource{
        .header = header,
        .source = ""
    };
}

}