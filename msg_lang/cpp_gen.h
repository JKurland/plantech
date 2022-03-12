#pragma once
#include "module.h"
#include <string>

namespace pt::msg_lang {

struct CppSource {
    std::string header;
    std::string source;
};

CppSource genCpp(const module::Module& module);

}