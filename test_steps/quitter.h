#pragma once

#include <future>

#include "test_utils/steps.h"
#include "core_messages/control.h"
#include "messages/messages.h"

namespace pt::testing {
template<>
struct HandlerCreator<Quitter, void> {
    auto makeContextArgs() {
        return Quitter{ProgramEnd{}, std::promise<int>{}};
    }
};
}
