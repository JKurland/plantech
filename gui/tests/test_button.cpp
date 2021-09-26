#include <gtest/gtest.h>

#include "gui/button.h"
#include "gui/gui.h"

using namespace pt;


class TestButton: public ::testing::Test {
protected:
    void assertUnclicked(GuiHandle<Button> handle) {
        ASSERT_FALSE(gui.get(handle).clicked);
    }

    void assertClicked(GuiHandle<Button> handle) {
        ASSERT_TRUE(gui.get(handle).clicked);
    }

    void mouseButtonDown(auto handle) {
        gui.mouseButtonDown(handle);
    }

    void mouseButtonUp(auto handle) {
        gui.mouseButtonUp(handle);
    }

    auto addButton() {return gui.add(Button(), gui.root());}
    Gui gui;
};

TEST_F(TestButton, should_create_button_in_unclicked_state) {
    auto handle = addButton();
    assertUnclicked(handle);
}

TEST_F(TestButton, should_set_button_to_clicked_on_mouse_button_down) {
    auto handle = addButton();
    mouseButtonDown(handle);
    assertClicked(handle);
}

TEST_F(TestButton, should_set_button_to_unclicked_after_mouse_button_up) {
    auto handle = addButton();
    mouseButtonDown(handle);
    mouseButtonUp(handle);
    assertUnclicked(handle);
}


TEST_F(TestButton, should_set_button_to_unclicked_after_mouse_button_up_on_a_different_element) {
    auto handle = addButton();
    mouseButtonDown(handle);
    mouseButtonUp(gui.root());
    assertUnclicked(handle);
}
