#include "test_utils/test.h"

#include "test_steps/context.h"
#include "test_steps/rendering.h"
#include "test_steps/gui.h"
#include "test_steps/window.h"
#include "test_steps/quitter.h"

#include <gtest/gtest.h>

using namespace pt;
using namespace pt::testing;


auto setup() {
    return pt::testing::Test()
        (GivenAContext<Quitter, Window, VulkanRendering, GuiRenderer, GuiManager>());
}

TEST(TestTranslate, test_button_click_on_translated_button) {
    setup()
        (GivenAButton<"Button">())
        (WhenButtonClicked<"Button">())
        (ThenButtonClickedEventIsEmitted<"Button">())
        .run();
}

TEST(TestTranslate, test_no_click_no_event) {
    setup()
        (GivenAButton<"Button">())
        (Not(ThenButtonClickedEventIsEmitted<"Button">()))
        .run();
}

TEST(TestTranslate, test_button_click_not_on_translated_button) {
    setup()
        (GivenAButton<"Button">())
        (WhenClickNotOnButton<"Button">())
        (Not(ThenButtonClickedEventIsEmitted<"Button">()))
        .run();
}

TEST(TestTranslate, test_draw_multiple_frames) {
    setup()
        (GivenAButton<"B">())
        (WhenFramesDrawn())
        // then it doesn't crash
        .run();
}

TEST(TestTranslate, test_draw_multiple_buttons) {
    setup()
        (GivenAButton<"B">())
        (GivenAButton<"A">())
        // then it doesn't crash
        .run();
}

TEST(TestTranslate, test_click_one_button_of_two) {
    setup()
        (GivenAButton<"A">())
        (GivenAButton<"B">())
        (GivenButtonsDoNotOverlap<"A", "B">())
        (WhenButtonClicked<"A">())
        (ThenButtonClickedEventIsEmitted<"A">())
        (Not(ThenButtonClickedEventIsEmitted<"B">()))
        .run();
}
