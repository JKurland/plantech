#pragma once

#include <gtest/gtest.h>
#include <future>

#include "framework/context.h"
#include "core_messages/control.h"

namespace pt {

template<typename DerivedT>
class ProgramControlFixture: public ::testing::Test {
protected:
    ProgramControlFixture():
        p(),
        f(p.get_future()),
        quitter{ProgramEnd{}, std::move(p)}
    {}

    void startProgram() {
        getContext().emit_sync(ProgramStart{});
    }

    void quitProgram() {
        getContext().emit_sync(QuitRequested{0});
        ASSERT_EQ(f.get(), 0);
        getContext().no_more_messages();
        getContext().wait_for_all_events_to_finish();
    }

    Quitter&& takeQuitter() {
        return std::move(quitter);
    }
private:
    std::promise<int> p;
    std::future<int> f;
    Quitter quitter;

    auto& getContext() {
        return dynamic_cast<DerivedT*>(this)->context;
    }
};

}
