#pragma once

#include "messages/messages.h"
#include "test_utils/test.h"

#include <functional>
#include <type_traits>
#include <optional>
#include <any>

namespace pt::testing {

struct ProjectComponentInfo {
    void* addComponentForEntity;
    void* getComponentForEntity;
    void* removeComponentForEntity;
};

template<typename ContextT>
using AddComponentForEntityT = void(*)(std::decay_t<ContextT>&, ProjectEntityHandle);

template<typename ContextT>
using GetComponentForEntityT = std::optional<std::any>(*)(std::decay_t<ContextT>&, ProjectEntityHandle);

template<typename ContextT>
using RemoveComponentForEntityT = void(*)(std::decay_t<ContextT>&, ProjectEntityHandle);

template<typename ContextT, typename ComponentT, ComponentT setValue>
ProjectComponentInfo makeComponentInfo() {
    using CT = std::decay_t<ContextT>;

    return ProjectComponentInfo{
        reinterpret_cast<void*>(static_cast<AddComponentForEntityT<CT>>([](CT& context, ProjectEntityHandle handle) {
            context.request_sync(SetProjectEntityComponent<ComponentT>{handle, setValue});
        })),

        reinterpret_cast<void*>(static_cast<GetComponentForEntityT<CT>>([](CT& context, ProjectEntityHandle handle) -> std::optional<std::any> {
            std::optional<ComponentT> component = context.request_sync(GetProjectEntityComponent<ComponentT>{handle});
            if (!component.has_value()) {
                return std::nullopt;
            }
            return std::any(*component);
        })),

        reinterpret_cast<void*>(static_cast<RemoveComponentForEntityT<CT>>([](CT& context, ProjectEntityHandle handle) {
            context.request_sync(RemoveProjectEntityComponent<ComponentT>{handle});
        }))
    };
}

template<WorldEntryName Name>
struct GivenAProjectComponent: TestStep<RequiresNameOnly<"Context">, Provides<Name, ProjectComponentInfo>> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        context.request_sync(RegisterProjectComponent<int>{});
        return this->worldUpdate(
            WorldEntry<Name, ProjectComponentInfo>{makeComponentInfo<decltype(context), int, 2>()}
        );
    }
};


template<WorldEntryName Name>
struct GivenAProjectEntity: TestStep<RequiresNameOnly<"Context">, Provides<Name, ProjectEntityHandle>> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        ProjectEntityHandle handle = context.request_sync(NewProjectEntity{});
        return this->worldUpdate(
            WorldEntry<Name, ProjectEntityHandle>{handle}
        );
    }
};


template<WorldEntryName EntityName, WorldEntryName ComponentName>
struct WhenProjectComponentSetOnEntity: TestStep<
    RequiresNameOnly<"Context">,
    Requires<EntityName, ProjectEntityHandle>,
    Requires<ComponentName, ProjectComponentInfo>
> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        const auto& componentInfo = world.template getEntry<ComponentName, ProjectComponentInfo>();
        auto addComponent = reinterpret_cast<AddComponentForEntityT<decltype(context)>>(componentInfo.addComponentForEntity);
        auto entityHandle = world.template getEntry<EntityName, ProjectEntityHandle>();
        addComponent(context, entityHandle);
    }  
};

template<WorldEntryName EntityName, WorldEntryName ComponentName>
struct ThenCanGetComponentForEntity: TestStep<
    RequiresNameOnly<"Context">,
    Requires<EntityName, ProjectEntityHandle>,
    Requires<ComponentName, ProjectComponentInfo>
> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        const auto& componentInfo = world.template getEntry<ComponentName, ProjectComponentInfo>();
        auto getComponent = reinterpret_cast<GetComponentForEntityT<decltype(context)>>(componentInfo.getComponentForEntity);
        auto entityHandle = world.template getEntry<EntityName, ProjectEntityHandle>();
        
        std::optional<std::any> component = getComponent(context, entityHandle);
        if (!component.has_value()) {
            throw "Not component for entity";
        }
    }
};


template<WorldEntryName ComponentName>
struct GivenAnUnregisteredComponent: TestStep<
    RequiresNameOnly<"Context">,
    Provides<ComponentName, ProjectComponentInfo>
> {
    struct UnregisteredComponent {};

    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();

        return this->worldUpdate(
            WorldEntry<ComponentName, ProjectComponentInfo>{makeComponentInfo<decltype(context), UnregisteredComponent, UnregisteredComponent{}>()}
        );
    }
};

template<WorldEntryName EntityName>
struct WhenEntityRemoved: TestStep<
    RequiresNameOnly<"Context">,
    Requires<EntityName, ProjectEntityHandle>
> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        auto entity = world.template getEntry<EntityName, ProjectEntityHandle>();

        context.request_sync(RemoveProjectEntity{entity});
    }
};

template<WorldEntryName EntityName, WorldEntryName ComponentName>
struct WhenProjectComponentRemovedFromEntity: TestStep<
    RequiresNameOnly<"Context">,
    Requires<EntityName, ProjectEntityHandle>,
    Requires<ComponentName, ProjectComponentInfo>
> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        auto entity = world.template getEntry<EntityName, ProjectEntityHandle>();
        const auto& componentInfo = world.template getEntry<ComponentName, ProjectComponentInfo>();

        auto removeComponent = reinterpret_cast<RemoveComponentForEntityT<decltype(context)>>(componentInfo.removeComponentForEntity);
        removeComponent(context, entity);
    }
};

struct WhenEntityAdded: TestStep<RequiresNameOnly<"Context">> {
    auto step(auto world) const {
        auto& context = world.template getEntryByName<"Context">();
        context.request_sync(NewProjectEntity{});
    }
};


}