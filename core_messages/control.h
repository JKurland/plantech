#pragma once

#include "framework/context.h"
#include <future>
#include "messages/messages.h"



namespace pt {

struct Quitter {
    EVENT(QuitRequested) {
        if constexpr (ctx.template can_handle<ProgramEnd>()) {
            co_await ctx.emit_await(pe);
        } 
        release_main.set_value(event.exit_status);
        co_return;
    }

    ProgramEnd pe;
    std::promise<int> release_main;
};

}
