#include <gtest/gtest.h>


#include "core_messages/control.h"
#include "framework/context.h"
#include "gui_rendering/gui_renderer.h"
#include "gui/gui.h"
#include "gui/mesh.h"
#include "rendering/vulkan.h"
#include "test_utils/fixture.h"
#include "window/window.h"

using namespace pt;

struct MockGuiManager {
    REQUEST(GetGui) {
        co_return *gui;
    }

    Gui* gui;
};

class TestMesh: public ProgramControlFixture<TestMesh> {
protected:
    TestMesh():
        ProgramControlFixture(),
        gui(),
        meshSet(gui.add(MeshSet{glm::mat4(1.0)}, gui.root())),
        context(make_context(
            takeQuitter(),
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(),
            MockGuiManager{&gui}
        ))
    {
    }

    void newFrame() {
        context.emit_sync(NewFrame{});
    }

    auto addMesh(std::vector<glm::vec3> points) {
        return gui.add(GuiMesh(std::move(points)), meshSet);
    }

    friend class ProgramControlFixture<TestMesh>;
private:
    // we don't call it in these tests but Window needs it to be happy
    std::function<void()> pollWindow;
    Gui gui;
    GuiHandle<MeshSet> meshSet;
    Context<
        Quitter,
        Window,
        VulkanRendering,
        GuiRenderer,
        MockGuiManager
    > context;
};

TEST_F(TestMesh, test_render_mesh) {
    startProgram();
    addMesh(std::vector<glm::vec3>{{20, 20, 0}, {100, 100, 0}, {20, 100, 0}});
    newFrame();
    quitProgram();
}
