#include "cpp_gen.h"

#include "module.h"
#include <iostream>
#include <vector>
#include <string>

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

std::string dumpItemName(const ItemName& name) {
    std::string rtn = name.path[0];
    for (size_t i = 1; i < name.path.size(); i++) {
        rtn.append("::");
        rtn.append(name.path[i]);
    }
    return rtn;
}

std::string dumpDataType(const module::DataType& dataType, const module::Module& module) {
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
        return "1__ERROR__";
    } else if (dataType.template is<module::TemplateParameter>()) {
        return dataType.template get<module::TemplateParameter>().name;
    } else if (dataType.template is<module::TemplateMemberType>()) {
        const auto& t = dataType.template get<module::TemplateMemberType>();
        std::string rtn = "typename ";
        rtn.append(t.param.name);
        for (const auto& p: t.path) {
            rtn.append("::");
            rtn.append(p);
        }
        return rtn;
    } else if (dataType.template is<module::ImportedType>()) {
        return dumpItemName(dataType.template get<module::ImportedType>().name);
    } else if (dataType.template is<module::TemplateInstance>()) {
        std::string rtn;
        const auto& templateInstance = dataType.template get<module::TemplateInstance>();
        rtn.append(dumpDataType(*templateInstance.template_, module));
        rtn.push_back('<');
        bool skipComma = true;
        for (const auto& arg: templateInstance.args) {
            if (!skipComma) {
                rtn.append(", ");
            }
            skipComma = false;
            rtn.append(dumpDataType(arg, module));
        }
        rtn.push_back('>');
        return rtn;
    }

    assert(dataType.template is<module::MessageHandle>());
    const ItemName& name = module.getMessage(dataType.template get<module::MessageHandle>()).name;
    return dumpItemName(name);
};

CppSource genCpp(const module::Module& module, const std::vector<std::string>& includeHeaders) {
    std::string header;


    header.append("#include <string>\n");
    header.append("#include <cstdint>\n");

    for (const auto& h: includeHeaders) {
        header.append("#include \"");
        header.append(h);
        header.append("\"\n");
    }

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
        header.append(dumpItemName(message.name));
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
        header.append(dumpItemName(message.name));
        header.append(" {\n");
        for (const auto& member: message.members) {
            header.append("    ");
            header.append(dumpDataType(member.type, module));
            header.push_back(' ');
            header.append(member.name);
            header.append(";\n");
        }

        if (message.expectedResponse) {
            header.append("    using ResponseT = ");
            header.append(dumpDataType(*message.expectedResponse, module));
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