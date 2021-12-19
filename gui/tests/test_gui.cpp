#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include "gui/gui.h"

using namespace pt;

struct IntElement: public GuiElement {
    using PosDataT = void;
    IntElement(int x): x(x) {}
    int x;
};

struct TargetElement: public GuiElement {
    using PosDataT = void;
    TargetElement(int target): target(target) {}
    int target;
};

using TestGui = GuiImpl<GuiRoot, IntElement, TargetElement>;

class TestGuiWalk: public ::testing::Test {
protected:
    TestGui gui;
};

struct WalkContext {
    int sum = 0;
};

struct TestVisitor {
    void operator()(const TestGui&, const GuiHandle<GuiRoot>&, WalkContext) {}
    WalkContext operator()(const TestGui& gui, const GuiHandle<IntElement>& e, WalkContext c) {
        int x = gui.get(e).x;
        c.sum += x;
        return c;
    }

    void operator()(const TestGui& gui, const GuiHandle<TargetElement>& e, const WalkContext& c) {
        ASSERT_EQ(c.sum, gui.get(e).target);
    }
};

TEST_F(TestGuiWalk, test_visit_all_with_context) {
    auto i = gui.add(IntElement(1), gui.root());
    gui.add(TargetElement(1), i);
    gui.visitAllWithContext(TestVisitor{}, WalkContext{});
}

TEST_F(TestGuiWalk, test_visit_all_with_context2) {
    auto i1 = gui.add(IntElement(1), gui.root());
    auto i2 = gui.add(IntElement(2), i1);
    gui.add(TargetElement(3), i2);
    gui.visitAllWithContext(TestVisitor{}, WalkContext{});
}

TEST_F(TestGuiWalk, test_visit_all_with_context3) {
    auto i1 = gui.add(IntElement(1), gui.root());
    auto i2 = gui.add(IntElement(2), i1);
    auto i3 = gui.add(IntElement(3), i1);
    gui.add(TargetElement(3), i2);
    gui.add(TargetElement(4), i3);
    gui.visitAllWithContext(TestVisitor{}, WalkContext{});
}

TEST_F(TestGuiWalk, test_visit_all_with_context4) {
    auto i1 = gui.add(IntElement(1), gui.root());
    auto i2 = gui.add(IntElement(2), i1);
    auto i3 = gui.add(IntElement(3), i1);
    auto i4 = gui.add(IntElement(10), i3);

    gui.add(TargetElement(3), i2);
    gui.add(TargetElement(3), i2);
    gui.add(TargetElement(3), i2);

    gui.add(TargetElement(4), i3);
    gui.add(TargetElement(4), i3);
    gui.add(TargetElement(4), i3);

    gui.add(TargetElement(14), i4);
    gui.add(TargetElement(14), i4);
    gui.visitAllWithContext(TestVisitor{}, WalkContext{});
}
