#pragma once

#include "framework/context.h"
#include <future>

namespace pt {
    class ProgramEnd;
    class ProgramStart;
}


namespace pt {


// Handler constructors have all just finished, load things that couldn't be loaded during
// construction.
class ProgramStart {
};

// The program is about to end, you can still talk to other handlers
// after this event has been emitted and you must stay capable of responding
// to the requests of others. The point is to get yourself in order to be
// destructed, since during destruction you won't be able to make new requests.
// Once all handlers have returned from EVENT(ProgramEnd) the destructors will be called
// and main exits.
class ProgramEnd {
};

// Something has requested a quit, this doesn't necessarily mean the program will actually quit
struct QuitRequested {
    int exit_status;
};


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
