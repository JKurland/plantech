#pragma once
#include "messages/messages.h"

#include "framework/context.h"

#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <any>

namespace pt {

class Project {
public:
    REQUEST(NewProjectEntity) {
        ProjectEntityHandle handle{nextEntityHandle};
        nextEntityHandle++;
        co_return handle;
    }

    REQUEST(RemoveProjectEntity) {
        for (auto& kv: entityComponentMaps) {
            auto& map = kv.second;

            map.erase(request.entity.idx);
        }
        co_return;
    }

    TEMPLATE_REQUEST(GetProjectEntityComponent<ComponentT>, typename ComponentT) {
        std::type_index component = typeid(ComponentT);
        auto it = entityComponentMaps.find(component);

        if (it == entityComponentMaps.end()) {
            co_return std::nullopt;
        }

        auto& entityMap = it->second;

        auto it2 = entityMap.find(request.entity.idx);

        if (it2 == entityMap.end()) {
            co_return std::nullopt;
        }

        co_return std::any_cast<ComponentT>(it2->second);
    }

    TEMPLATE_REQUEST(SetProjectEntityComponent<ComponentT>, typename ComponentT) {
        std::type_index component = typeid(ComponentT);

        auto it = entityComponentMaps.find(component);
        if (it == entityComponentMaps.end()) {
            co_return false;
        }

        it->second.insert_or_assign(request.entity.idx, std::move(request.component));
        co_return true;
    }

    TEMPLATE_REQUEST(RemoveProjectEntityComponent<ComponentT>, typename ComponentT) {
        std::type_index component = typeid(ComponentT);

        auto it = entityComponentMaps.find(component);
        if (it == entityComponentMaps.end()) {
            co_return;
        }

        it->second.erase(request.entity.idx);
    }

    TEMPLATE_REQUEST(RegisterProjectComponent<ComponentT>, typename ComponentT) {
        std::type_index component = typeid(ComponentT);
        entityComponentMaps[component];
        co_return;
    }
private:
    std::unordered_map<std::type_index, std::unordered_map<size_t, std::any>> entityComponentMaps;
    size_t nextEntityHandle = 0;
};

}
