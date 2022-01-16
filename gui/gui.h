#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <compare>
#include <bit>
#include <concepts>
#include <functional>

#include "utils/template_meta.h"
#include "gui/element.h"
#include "gui/button.h"
#include "gui/translate.h"
#include "gui/mesh.h"

namespace pt {

class GuiRoot: public GuiElement {
public:
    using PosDataT = void;
};

template<typename...Ts>
class GuiImpl;

// a handle can be used with either the Gui object that created it or a Gui object
// that is a copy of the original Gui object
template<typename...Ts>
class GuiHandle;

template<typename T>
class GuiHandle<T> {
public:
    auto operator<=>(const GuiHandle<T>&) const = default;

private:
    explicit GuiHandle(size_t index): index(index) {}

    size_t index;
    template<typename...Ts>
    friend class GuiImpl;
    friend class std::hash<GuiHandle>;
};

template<typename First, typename Second, typename...Ts>
class GuiHandle<First, Second, Ts...> {
public:
    auto operator<=>(const GuiHandle<First, Second, Ts...>&) const = default;

private:
    GuiHandle(size_t index, size_t typeIndex): index(index), typeIndex(typeIndex) {}


    size_t index;
    size_t typeIndex;
    template<typename...Us>
    friend class GuiImpl;
    friend class std::hash<GuiHandle>;
};
} // pt

namespace std {
    template<typename...Ts>
    struct hash<::pt::GuiHandle<Ts...>> {
        constexpr size_t operator()(const ::pt::GuiHandle<Ts...>& handle) const noexcept {
            if constexpr (sizeof...(Ts) == 1) {
                return handle.index;
            } else {
                return handle.index ^ std::rotl(handle.typeIndex, sizeof(handle.typeIndex)*4);
            }
        }
    };
}

namespace pt {
template<typename...ElemTs>
class GuiImpl {
public:
    using Handle = GuiHandle<ElemTs...>;

    GuiImpl();

    template<typename T, typename...Ts>
    GuiHandle<T> add(T element, const GuiHandle<Ts...>& parent);

    static GuiHandle<GuiRoot> root();

    template<typename T>
    T& get(GuiHandle<T> handle);

    template<typename T>
    const T& get(GuiHandle<T> handle) const;

    template<typename...Ts>
    void mouseButtonDown(const GuiHandle<Ts...>& target);

    template<typename...Ts>
    void mouseButtonUp(const GuiHandle<Ts...>& target);

    template<typename T, typename F>
    auto visit(const GuiHandle<T>& handle, F&& f) const;

    template<typename F>
    auto visit(const GuiHandle<ElemTs...>& handle, F&& f) const;

    template<typename T, typename F>
    auto visit(const GuiHandle<T>& handle, F&& f);

    template<typename F>
    auto visit(const GuiHandle<ElemTs...>& handle, F&& f);

    template<typename F>
    void visitAll(F&& f) const;

    template<typename T, typename F>
    void visitAllType(F&& f) const;

    // visits each element in the gui in topological order.
    // F should be callable with the following arguments 
    // (const Gui&, const GuiHandle<ELEMENT_TYPE>&, const ContextT&). The idea
    // with the context is that the context seen by a node in the tree depends
    // on all the nodes above it. For example a node's parent can modify the
    // context that node sees. To modify the context F can return a ContextT
    // (it can also return void and not modify the context). The actual order
    // the nodes are visited in is not defined but the context argument for each
    // node will be the context returned when its parent was visited (or its
    // parent's parent if F returned void for the parent, and so on)
    template<typename F, typename ContextT>
    void visitAllWithContext(F&& f, ContextT initContext) const;

    template<typename F, typename ContextT>
    ContextT visitWithContext(F&& f, const GuiHandle<ElemTs...>& handle, const ContextT& context) const;

    template<typename...Ts>
    static GuiHandle<ElemTs...> convertHandle(const GuiHandle<Ts...>& handle);
private:
    struct Node {
        GuiHandle<ElemTs...> parent;
        std::vector<GuiHandle<ElemTs...>> children;
    };

    size_t nextHandleIndex;
    GuiHandle<ElemTs...> mouseFocusedNode;

    std::tuple<std::unordered_map<size_t, ElemTs>...> elements;
    std::unordered_map<GuiHandle<ElemTs...>, Node> nodes;


    Node& getNode(const GuiHandle<ElemTs...>& handle);
    const Node& getNode(const GuiHandle<ElemTs...>& handle) const;

    template<typename T>
    T& get(size_t index);

    template<typename T>
    const T& get(size_t index) const;
};


template<typename...ElemTs>
GuiImpl<ElemTs...>::GuiImpl(): nextHandleIndex(1), mouseFocusedNode(convertHandle(root())) {
    std::get<ppFindType<GuiRoot, ElemTs...>()>(elements).emplace(root().index, GuiRoot{});
    nodes.emplace(
        convertHandle(root()),
        Node{
            convertHandle(root()),
            {}
        }
    );
}


// static 
template<typename...ElemTs>
template<typename...Ts>
GuiHandle<ElemTs...> GuiImpl<ElemTs...>::convertHandle(const GuiHandle<Ts...>& handle) {
    if constexpr (sizeof...(Ts) == 1) {
        return {handle.index, ppFindType<Ts..., ElemTs...>()};
    } else {
        return {handle.index, handle.typeIndex};
    }
}

template<typename...ElemTs>
template<typename T, typename...Ts>
GuiHandle<T> GuiImpl<ElemTs...>::add(T element, const GuiHandle<Ts...>& parent) {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    const GuiHandle<ElemTs...> convertedParent = convertHandle(parent);

    std::get<typeIndex>(elements).emplace(nextHandleIndex, std::move(element));

    nodes.emplace(
        GuiHandle<ElemTs...>{nextHandleIndex, typeIndex},
        Node {
            convertedParent,
            {}
        }
    );

    GuiHandle<T> newHandle(nextHandleIndex);
    nextHandleIndex++;

    getNode(convertedParent).children.push_back(convertHandle(newHandle));
    return newHandle;
}


template<typename...ElemTs>
GuiImpl<ElemTs...>::Node& GuiImpl<ElemTs...>::getNode(const GuiHandle<ElemTs...>& handle) {
    return nodes.find(handle)->second;
}

template<typename...ElemTs>
const GuiImpl<ElemTs...>::Node& GuiImpl<ElemTs...>::getNode(const GuiHandle<ElemTs...>& handle) const {
    return nodes.find(handle)->second;
}

// static
template<typename...ElemTs>
GuiHandle<GuiRoot> GuiImpl<ElemTs...>::root() {
    return GuiHandle<GuiRoot>{0};
}


template<typename...ElemTs>
template<typename T>
T& GuiImpl<ElemTs...>::get(GuiHandle<T> handle) {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    return std::get<typeIndex>(elements).find(handle.index)->second;
}

template<typename...ElemTs>
template<typename T>
const T& GuiImpl<ElemTs...>::get(GuiHandle<T> handle) const {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    return std::get<typeIndex>(elements).find(handle.index)->second;
}

template<typename...ElemTs>
template<typename T>
T& GuiImpl<ElemTs...>::get(size_t index) {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    return std::get<typeIndex>(elements).find(index)->second;
}

template<typename...ElemTs>
template<typename T>
const T& GuiImpl<ElemTs...>::get(size_t index) const {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    return std::get<typeIndex>(elements).find(index)->second;
}


template<typename...ElemTs>
template<typename T, typename F>
auto GuiImpl<ElemTs...>::visit(const GuiHandle<T>& handle, F&& f) const {
    return f(get(handle));
}

template<typename...ElemTs>
template<typename F>
auto GuiImpl<ElemTs...>::visit(const GuiHandle<ElemTs...>& handle, F&& f) const {
    using RetT = std::invoke_result_t<F&&, const PpFirstType<ElemTs...>&>;
    constexpr RetT (*fTable[])(const GuiImpl&, const GuiHandle<ElemTs...>& handle, F&& f) = {
        [](const GuiImpl& gui, const GuiHandle<ElemTs...>& handle, F&& f) {return std::invoke(std::forward<F>(f), gui.get<ElemTs>(handle.index));}
        ...
    };
    return fTable[handle.typeIndex](*this, handle, std::forward<F>(f));
}

template<typename...ElemTs>
template<typename T, typename F>
auto GuiImpl<ElemTs...>::visit(const GuiHandle<T>& handle, F&& f) {
    return f(get(handle));
}

template<typename...ElemTs>
template<typename F>
auto GuiImpl<ElemTs...>::visit(const GuiHandle<ElemTs...>& handle, F&& f) {
    using RetT = std::invoke_result_t<F&&, PpFirstType<ElemTs...>&>;
    constexpr RetT (*fTable[])(GuiImpl&, const GuiHandle<ElemTs...>& handle, F&& f) = {
        [](GuiImpl& gui, const GuiHandle<ElemTs...>& handle, F&& f) {return std::invoke(std::forward<F>(f), gui.get<ElemTs>(handle.index));}
        ...
    };
    return fTable[handle.typeIndex](*this, handle, std::forward<F>(f));
}


template<typename...ElemTs>
template<typename F>
void GuiImpl<ElemTs...>::visitAll(F&& f) const {
    (visitAllType<ElemTs>(f),...);
}

template<typename...ElemTs>
template<typename T, typename F>
void GuiImpl<ElemTs...>::visitAllType(F&& f) const {
    constexpr size_t typeIndex = ppFindType<T, ElemTs...>();
    for (const auto& it: std::get<typeIndex>(elements)) {
        f(*this, GuiHandle<T>(it.first));
    }
}

template<typename...ElemTs>
template<typename F, typename ContextT>
ContextT GuiImpl<ElemTs...>::visitWithContext(F&& f, const GuiHandle<ElemTs...>& handle, const ContextT& context) const {
    constexpr ContextT (*fTable[])(const GuiImpl&, const GuiHandle<ElemTs...>&, const ContextT&, F&&) = {
        [](const GuiImpl& gui, const GuiHandle<ElemTs...>& handle, const ContextT& context, F&& f) {
            using ResultT = std::invoke_result_t<F, GuiImpl, GuiHandle<ElemTs>, ContextT>;
            if constexpr (std::is_void_v<ResultT>) {
                std::invoke(std::forward<F>(f), gui, GuiHandle<ElemTs>(handle.index), context);
                return context;
            } else {
                return std::invoke(std::forward<F>(f), gui, GuiHandle<ElemTs>(handle.index), context);
            }
        }
        ...
    };
    return fTable[handle.typeIndex](*this, handle, context, std::forward<F>(f));
}

template<typename...ElemTs>
template<typename F, typename ContextT>
void GuiImpl<ElemTs...>::visitAllWithContext(F&& f, ContextT initContext) const {
    std::vector<Handle> toVisit;
    toVisit.push_back(convertHandle(root()));

    // the size_t is the remaining nodes this context needs to be passed to in total.
    // initContext only needs to be passed to the root node, so it starts at 1.
    std::vector<std::pair<ContextT, size_t>> contexts;
    contexts.push_back(std::make_pair(initContext, 1));

    while (!toVisit.empty()) {
        Handle& thisHandle = toVisit.back();
        auto& thisContext = contexts.back();

        ContextT newContext = visitWithContext(f, thisHandle, thisContext.first);
        thisContext.second--;

        const Node& node = getNode(thisHandle);

        toVisit.pop_back();
        if (node.children.empty()) {
            while (!contexts.empty() && contexts.back().second == 0) {
                contexts.pop_back();
            }
        } else {
            toVisit.insert(toVisit.end(), node.children.begin(), node.children.end());
            contexts.push_back(std::make_pair(newContext, node.children.size()));
        }
    }
}

template<typename...ElemTs>
template<typename...Ts>
void GuiImpl<ElemTs...>::mouseButtonDown(const GuiHandle<Ts...>& target) {
    visit(target, [](auto& elem){elem.mouseButtonDown();});
    mouseFocusedNode = convertHandle(target);
}

template<typename...ElemTs>
template<typename...Ts>
void GuiImpl<ElemTs...>::mouseButtonUp(const GuiHandle<Ts...>& target) {
    auto f = [](auto& elem){elem.mouseButtonUp();};
    visit(target, f);
    visit(mouseFocusedNode, f);
    mouseFocusedNode = convertHandle(root());
}

using Gui = GuiImpl<
    GuiRoot,
    Button,
    Translate,
    MeshSet,
    GuiMesh
>;

struct GetGui {
    using ResponseT = Gui;
};
}
