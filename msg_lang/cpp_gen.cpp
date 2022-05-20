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
    return dataType.visit(
        [&](const module::BuiltinType& b) -> std::string {
            switch (b) {
                case module::BuiltinType::Int: {
                    return "std::int32_t";
                }
                case module::BuiltinType::UInt: {
                    return "std::uint32_t";
                }
                case module::BuiltinType::USize: {
                    return "std::size_t";
                }
                case module::BuiltinType::Bool: {
                    return "bool";
                }
                case module::BuiltinType::Void: {
                    return "void";
                }
                case module::BuiltinType::Byte: {
                    return "unsigned char";
                }
                case module::BuiltinType::Float: {
                    return "float";
                }
                case module::BuiltinType::String: {
                    return "std::string";
                }
                case module::BuiltinType::List: {
                    return "std::vector";
                }
                case module::BuiltinType::Option: {
                    return "std::optional";
                }
                case module::BuiltinType::Double: {
                    return "double";
                }
            }
            // unreachable
            assert(false);
            return "2__ERROR__";
        },
        [&](const module::ErrorDataType&) -> std::string {return "1__ERROR__";},
        [&](const module::TemplateParameter& p) -> std::string {return p.name;},
        [&](const module::TemplateMemberType& t) {
            std::string rtn = "typename ";
            rtn.append(t.param.name);
            for (const auto& p: t.path) {
                rtn.append("::");
                rtn.append(p);
            }
            return rtn;
        },
        [&](const module::ImportedType& i) -> std::string {return dumpItemName(i.name);},
        [&](const module::TemplateInstance& i) {
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
        },
        [&](const module::MessageHandle& h) {
            return dumpItemName(module.getMessage(h).name);
        }
    );
};

CppSource genCpp(const module::Module& module, const std::vector<std::string>& includeHeaders, const std::vector<std::string>& systemHeaders) {
    std::string header;

    header.append("#pragma once\n");
    header.append("#include <string>\n");
    header.append("#include <cstdint>\n");
    header.append("#include <vector>\n");
    header.append("#include <optional>\n");

    for (const auto& h: systemHeaders) {
        header.append("#include <");
        header.append(h);
        header.append(">\n");
    }

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
    std::vector<module::MessageHandle> messages = module.topologicalOrder();
    for (auto it = messages.rbegin(); it != messages.rend(); it++) {
        const auto& message = module.getMessage(*it);
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

        // if this message has no imported members we can safely add comparison operators
        const bool hasImportedMembers = [&]{
            for (const auto& member: message.members) {
                if (member.type.template is<module::ImportedType>()) {
                    return true;
                }
            }
            return false;
        }();

        if (!hasImportedMembers) {
            header.append("auto operator<=>(const ");
            header.append(dumpItemName(message.name));
            header.append("&) const = default;\n");
        }

        if (!hasImportedMembers) {
            header.append("bool operator==(const ");
            header.append(dumpItemName(message.name));
            header.append("&) const = default;\n");
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