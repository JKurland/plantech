#pragma once

#include <tuple>

#include "test_utils/test.h"

#include "framework/context.h"

namespace pt::testing {

// Specialisations need to have:
//      auto makeContextArgs() 
//          the output is given to makeContext. The HandlerCreator is kept alive longer than
//          the context it helped create so it can hold resources that the context
//          uses.
//
template<typename HandlerT, typename Enable=void>
class HandlerCreator;

// Default constructible handlers just get default constructed
template<typename HandlerT>
class HandlerCreator<HandlerT, std::enable_if_t<std::is_default_constructible_v<HandlerT>>> {
public:
    auto makeContextArgs() const {
        return HandlerT{};
    }
};

template<typename...HandlerTs>
class GivenAContext: public TestStep<Provides<"Context", Context<HandlerTs...>>> {
public:
    auto step(auto world) {
        return this->worldUpdate(
            WorldEntry<"Context", Context<HandlerTs...>>{
                makeContext(std::get<HandlerCreator<HandlerTs>>(handlerCreators).makeContextArgs()...)
            };
        );
    }

    void cleanup(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        if constexpr (context.template can_handle<QuitRequested>()) {
            context.emit_sync(QuitRequested{0});
        }
        context.no_more_messages();
        context.wait_for_all_events_to_finish();
    }

private:
    std::tuple<HandlerCreator<HandlerTs>...> handlerCreators;
};

}
