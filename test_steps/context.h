#pragma once

#include <tuple>

#include "test_utils/test.h"

#include "framework/context.h"
#include "messages/messages.h"

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

class SentMessages {
private:
    class Observer;
public:
    Observer& observer() {
        return *observer_;
    }

    SentMessages(): messages(new std::vector<std::pair<void*, std::type_index>>), observer_(new Observer(*messages)) {}

    template<typename MessageT, typename PredT>
    bool hasMessageMatchingPred(const PredT& pred) const {
        for (const auto& pair : *messages) {
            if (typeid(MessageT) == pair.second && pred(*static_cast<MessageT*>(pair.first))) {
                return true;
            }
        }
        return false;
    }
private:

    class Observer: public ContextObserver {
    public:
        void event(void* event, std::type_index type) override {
            messages->push_back(std::make_pair(event, type));
        }

        Observer(std::vector<std::pair<void*, std::type_index>>& messages): messages(&messages) {}
    private:
        std::vector<std::pair<void*, std::type_index>>* messages;    
    };

    std::unique_ptr<std::vector<std::pair<void*, std::type_index>>> messages;
    std::unique_ptr<Observer> observer_;
};

template<typename...HandlerTs>
class GivenAContext: public TestStep<
    Provides<"Context", Context<HandlerTs...>>,
    Provides<"SentMessages", SentMessages>
> {
public:
    auto step(auto world) {
        auto context = make_context(std::get<HandlerCreator<HandlerTs>>(handlerCreators).makeContextArgs()...);
        auto sentMessages = SentMessages{};
        context.addObserver(sentMessages.observer());

        context.emit_sync(ProgramStart{});
        return this->worldUpdate(
            WorldEntry<"Context", Context<HandlerTs...>>{
                std::move(context)
            },
            WorldEntry<"SentMessages", SentMessages>{
                std::move(sentMessages)
            }
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
