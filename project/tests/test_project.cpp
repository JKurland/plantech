#include "test_utils/test.h"
#include "test_steps/context.h"
#include "test_steps/project.h"
#include "project/project.h"

#include <gtest/gtest.h>

using namespace pt;
using namespace pt::testing;

auto setup() {
    return pt::testing::Test()
        (GivenAContext<Project>());
}

TEST(TestProject, add_and_retrieve_component) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (ThenCanGetComponentForEntity<"A", "C">())
    .run();
}

TEST(TestProject, get_component_should_fail_when_not_set) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (Not(ThenCanGetComponentForEntity<"A", "C">()))
    .run();
}

TEST(TestProject, get_component_should_fail_when_unregisted_component_set) {
    setup()
        (GivenAProjectEntity<"A">())
        (GivenAnUnregisteredComponent<"C">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (Not(ThenCanGetComponentForEntity<"A", "C">()))
    .run();
}

TEST(TestProject, entities_are_kept_separate) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (GivenAProjectEntity<"B">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (ThenCanGetComponentForEntity<"A", "C">())
        (Not(ThenCanGetComponentForEntity<"B", "C">()))
    .run();
}

TEST(TestProject, get_component_on_deleted_entity) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (WhenEntityRemoved<"A">())
        (Not(ThenCanGetComponentForEntity<"A", "C">()))
    .run();
}

TEST(TestProject, add_and_remove_component) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (WhenProjectComponentRemovedFromEntity<"A", "C">())
        (Not(ThenCanGetComponentForEntity<"A", "C">()))
    .run();
}

TEST(TestProject, get_component_on_deleted_entity_with_another_entity_added) {
    setup()
        (GivenAProjectComponent<"C">())
        (GivenAProjectEntity<"A">())
        (WhenProjectComponentSetOnEntity<"A", "C">())
        (WhenEntityRemoved<"A">())
        (WhenEntityAdded())
        (Not(ThenCanGetComponentForEntity<"A", "C">()))
    .run();
}


