#pragma once

#include <tuple>
#include <string_view>
#include <utility>
#include <algorithm>
#include <concepts>
#include <cassert>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <compare>
#include <exception>
#include <array>
#include <optional>
#include <iostream>
#include <typeindex>

#include <gtest/gtest.h>

namespace pt::testing {

struct WorldEntryName {
    static constexpr size_t MaxN = 256;

    template<size_t N>
    constexpr WorldEntryName(const char (&s)[N]):name() {
        for (size_t i = 0; i < N; i++) {
            name[i] = s[i];
        }
        static_assert(N <= MaxN, "Too large a world entry name");
    }
    constexpr bool operator==(const WorldEntryName& o) const = default;


    std::array<char, MaxN> name;
};

template<WorldEntryName Name, typename ValueT_>
struct WorldEntry {
    using ValueT = ValueT_;
    ValueT value;
    static constexpr WorldEntryName name = Name;

    constexpr bool is(const WorldEntryName& o) const {return name == o;}
};

namespace detail {

    template<typename...Ts>
    struct TypeList {
        template<typename...NewTs>
        using AppendT = TypeList<Ts..., NewTs...>;

        template<typename...NewTs>
        using PrependT = TypeList<NewTs..., Ts...>;

        template<typename...OTs>
        auto operator+(TypeList<OTs...> o) {return AppendT<OTs...>{};}

        template<typename F>
        static void forEachType(F f) {
            if constexpr (sizeof...(Ts) > 0) {
                (f.template operator()<Ts>(), ...);
            }
        }
    private:

    };

    template<typename...Lists>
    auto concatTypeLists(Lists...lists) {return (TypeList<>{} + ... + lists);}

    template<typename...Lists>
    using ConcatTypeListsT = decltype(concatTypeLists(Lists{}...));





    template<typename...WorldEntryTs>
    class World {
    private:
        static constexpr size_t indexFromName(WorldEntryName name) {
            size_t idx = 0;
            size_t rtn = 0;
            size_t numFound = 0;

            ((WorldEntryTs::name == name ? rtn = idx, numFound++ : idx++), ...);
            if (numFound == 1) {
                return rtn;
            } else if (numFound > 1) {
                throw "more than one entry with name";
            } else {
                throw "name not in world";
            }
        }
    public:
        World() = default;

        template<WorldEntryName name, typename ValueT>
        constexpr const auto& getEntry() const {
            using EntryT = WorldEntry<name, ValueT>;
            return getEntry2<EntryT>();
        }

        template<WorldEntryName name, typename ValueT>
        constexpr auto& getEntry() {
            using EntryT = WorldEntry<name, ValueT>;
            return getEntry2<EntryT>();
        }

        template<WorldEntryName name, typename ValueT>
        void setEntry(ValueT&& value) {
            using EntryT = WorldEntry<name, std::decay_t<ValueT>>;
            setEntry2<EntryT>(std::forward<ValueT>(value));
        }

        // the *Entry2s are just the *Entrys but taking an EntryT instead of
        // Name and ValueT
        template<typename EntryT>
        constexpr const auto& getEntry2() const {
            if constexpr (hasEntry<EntryT>()) {
                assert(std::get<std::optional<EntryT>>(entries));
                return *std::get<std::optional<EntryT>>(entries)->value;
            } else {
                // this error will be caught by checkProvidesAndRequiresMatchUp
                // so just avoiding bad compiler errors
                return *(typename EntryT::ValueT*)(NULL);
            }
        }

        template<typename EntryT>
        constexpr auto& getEntry2() {
            if constexpr (hasEntry<EntryT>()) {
                assert(std::get<std::optional<EntryT>>(entries));
                return std::get<std::optional<EntryT>>(entries)->value;
            } else {
                // this error will be caught by checkProvidesAndRequiresMatchUp
                // so just avoiding bad compiler errors
                return *(typename EntryT::ValueT*)(NULL);
            }
        }

        template<typename EntryT>
        void setEntry2(EntryT&& entry) {
            if constexpr (hasEntry<std::decay_t<EntryT>>()) {
                std::get<std::optional<std::decay_t<EntryT>>>(entries) = std::forward<EntryT>(entry);
            }
        }

        template<WorldEntryName Name>
        using ValueTForName = typename std::tuple_element<indexFromName(Name), std::tuple<WorldEntryTs...>>;
    private:
        template<typename EntryT>
        static constexpr bool hasEntry() {
            return (std::is_same_v<WorldEntryTs, EntryT> || ...);
        }

        std::tuple<std::optional<WorldEntryTs>...> entries;
    };

}

template<typename...WorldEntryTs>
class WorldUpdate {
private:
    template<typename WorldEntryT>
    static constexpr bool containsEntry() {
        return (std::is_same_v<WorldEntryT, WorldEntryTs> || ...);
    }

    template<typename...ArgTs>
    static constexpr bool canConstructFrom() {
        if constexpr (sizeof...(ArgTs) != sizeof...(WorldEntryTs)) {
            return false;
        } else {
            return (containsEntry<std::decay_t<ArgTs>>() && ...);
        }
    }

public:
    template<typename...ArgTs, typename=std::enable_if_t<canConstructFrom<ArgTs...>()>>
    WorldUpdate(ArgTs&&...args) {
        ((std::get<std::optional<std::decay_t<ArgTs>>>(entries) = std::forward<ArgTs>(args)), ...);
    }

    template<typename WorldT>
    void pushUpdate(WorldT& world) && {
        (world.setEntry2(*std::get<std::optional<WorldEntryTs>>(entries)), ...);
    }
private:


    std::tuple<std::optional<WorldEntryTs>...> entries;
};


template<typename WorldT, typename...WorldEntryPolicies>
class WorldRef {
public:
    WorldRef(WorldT& world): world(&world) {}

    template<WorldEntryName Name, typename ValueT>
    auto& getEntry() {
        using EntryT = WorldEntry<Name, ValueT>;
        static_assert(getAllowed<EntryT>(), "Test step must request access to this entry via Provides parameter to base class");
        return world->template getEntry<Name, ValueT>();
    }

    template<WorldEntryName Name, typename ValueT>
    const auto& getEntry() const {
        using EntryT = WorldEntry<Name, ValueT>;
        static_assert(getAllowed<EntryT>(), "Test step must request access to this entry via Provides parameter to base class");
        return world->template getEntry<Name, ValueT>();
    }

    template<WorldEntryName Name>
    auto& getEntryByName() {
        return getEntry<Name, typename WorldT::ValueTForName<Name>>();
    }

    template<WorldEntryName Name>
    const auto& getEntryByName() const {
        return getEntry<Name, typename WorldT::ValueTForName<Name>>();
    }

    template<typename EntryT>
    static constexpr bool getAllowed() {
        return (WorldEntryPolicies::template allow<EntryT>() || ...);
    }
private:

    WorldT* world;
};


namespace detail {
    template<typename T>
    struct RequiresPolicy;

    template<typename WorldT, typename RequiresList>
    struct WorldRefFromRequires;

    template<typename WorldT, typename...Requires>
    struct WorldRefFromRequires<WorldT, TypeList<Requires...>> {
        using WorldRefT = WorldRef<WorldT, RequiresPolicy<Requires>...>;
    };

    template<typename WorldT, typename RequiresList>
    using WorldRefFromRequiresT = typename WorldRefFromRequires<WorldT, RequiresList>::WorldRefT;
}

template<typename...StepTs>
class Test {
public:
    Test() = default;
    ~Test() noexcept(false) {
        if (!used) {
            throw std::logic_error("Test was not used");
        }
    }

    Test(const Test&) = default;
    Test(Test&&) = default;

    Test& operator=(const Test&) = default;
    Test& operator=(Test&&) = default;

    template<typename NewStepT>
    auto operator()(NewStepT&& newStep) && {
        used = true;
        return Test<StepTs..., std::decay_t<NewStepT>>(std::tuple_cat(
            std::move(steps),
            std::tuple<std::decay_t<NewStepT>>(std::forward<NewStepT>(newStep))
        ));
    }

    template<typename NewStepT>
    auto operator()(NewStepT&& newStep) const & {
        used = true;
        return Test<StepTs..., std::decay_t<NewStepT>>(std::tuple_cat(
            steps,
            std::tuple<std::decay_t<NewStepT>>(std::forward<NewStepT>(newStep))
        ));
    }

    void run() {
        used = true;
        bool valid = true;
        checkProvidesAndRequiresMatchUp(valid);
        if (!valid) return;

        auto world = makeWorld();
        [&]<size_t...Is>(std::index_sequence<Is...>){
            (runStep(world, std::get<Is>(steps)), ...);
        }(std::make_index_sequence<sizeof...(StepTs)>{});


        [&]<size_t...Is>(std::index_sequence<Is...>){
            (runCleanup(world, std::get<sizeof...(Is) - Is - 1>(steps)), ...);
        }(std::make_index_sequence<sizeof...(StepTs)>{});
    }

    void markAsUsed() const {
        used = true;
    }
private:
    template<typename...ArgTs>
    Test(std::tuple<ArgTs...>&& steps): steps(std::move(steps)) {}

    template<typename...ArgTs>
    Test(const std::tuple<ArgTs...>& steps): steps(steps) {}

    template<typename...Ts>
    friend class Test;

    template<typename WorldT, typename StepT>
    void runStep(WorldT& world, const StepT& step) {
        auto worldRef = detail::WorldRefFromRequiresT<WorldT, detail::ConcatTypeListsT<typename StepT::Requires, typename StepT::RequiresNameOnly>>{world};

        using WorldUdpateT = std::decay_t<decltype(step.step(worldRef))>;
        if constexpr (std::is_void_v<WorldUdpateT>) {
            step.step(worldRef);
        } else {
            auto update = step.step(worldRef);
            std::move(update).pushUpdate(world);
        }
    }

    template<typename WorldT, typename StepT>
    void runCleanup(WorldT& world, const StepT& step) {
        auto worldRef = detail::WorldRefFromRequiresT<WorldT, detail::ConcatTypeListsT<typename StepT::Requires, typename StepT::RequiresNameOnly>>{world};
        step.cleanup(worldRef);
    }

    auto makeWorld() const {
        using WorldEntryTsList = detail::ConcatTypeListsT<typename StepTs::Provides...>;

        return []<typename...WorldEntryTs>(detail::TypeList<WorldEntryTs...>) {
            return detail::World<WorldEntryTs...>{};
        }(WorldEntryTsList{});
    }

    void checkProvidesAndRequiresMatchUp(bool& valid) const {
        valid = true;
        struct WorldEntryRuntime {
            std::string_view name;
            std::type_index valueType;

            bool operator==(const WorldEntryRuntime&) const = default;
        };

        auto makeWorldEntry = []<typename WorldEntryT> {
            return WorldEntryRuntime {
                std::string_view{WorldEntryT::name.name.data()},
                typeid(typename WorldEntryT::ValueT)
            };
        };

        std::vector<WorldEntryRuntime> providedEntries;

        auto processStep = [&]<typename StepT>() {
            if (!valid) return;

            std::vector<WorldEntryRuntime> unprovidedEntries;
            StepT::Requires::forEachType(
                [&]<typename RequiredEntryT>{
                    auto entry = makeWorldEntry.template operator()<RequiredEntryT>();
                    for (const auto& providedEntry: providedEntries) {
                        if (entry == providedEntry) return;
                    }
                    unprovidedEntries.push_back(entry);
                }
            );

            std::vector<std::string_view> unprovidedNameOnlyEntries;
            StepT::RequiresNameOnly::forEachType(
                [&]<typename RequiresNameOnlyT>{
                    for (const auto& providedEntry: providedEntries) {
                        if (providedEntry.name == RequiresNameOnlyT::name) return;
                    }
                    unprovidedNameOnlyEntries.push_back(RequiresNameOnlyT::name);
                }
            );

            if (!unprovidedEntries.empty() || !unprovidedNameOnlyEntries.empty()) {
                std::string error = "Some entries required by ";
                error.append(typeid(StepT).name());
                error.append(" have not been provided:");
                for (const auto& unprovidedEntry: unprovidedEntries) {
                    error.append("\n    ");
                    error.append(unprovidedEntry.name);
                    error.append(" -> ");
                    error.append(unprovidedEntry.valueType.name());
                }
                for (const auto& unprovidedEntry: unprovidedNameOnlyEntries) {
                    error.append("\n    ");
                    error.append(unprovidedEntry);
                }
                valid = false;
                ASSERT_TRUE(false) << error;
            }

            StepT::Provides::forEachType(
                [&]<typename Provides>{
                    providedEntries.push_back(makeWorldEntry.template operator()<Provides>());
                }
            );

        };

        detail::TypeList<StepTs...>::forEachType(processStep);
    }

    std::tuple<StepTs...> steps;

    mutable bool used = false;
};

Test() -> Test<>;


enum class ConstraintType {
    Provides,
    Requires,
    RequiresNameOnly,
};

template<WorldEntryName Name, typename ValueT>
struct Provides {
    static constexpr ConstraintType type = ConstraintType::Provides;
};

template<WorldEntryName Name, typename ValueT>
struct Requires {
    static constexpr ConstraintType type = ConstraintType::Requires;
};

// The name must be in world but we give no requirements for the type. This is to deal with
// Context which has a very complicated type.
template<WorldEntryName Name>
struct RequiresNameOnly {
    static constexpr ConstraintType type = ConstraintType::RequiresNameOnly;

    static constexpr WorldEntryName name = Name;
};


namespace detail {
    template<template<typename> typename Map, ConstraintType Filter, typename First, typename...WorldEntryDeclTs>
    auto filterTypesImpl() {
        if constexpr (First::type == Filter) {
            if constexpr (sizeof...(WorldEntryDeclTs) == 0) {
                return TypeList<typename Map<First>::type>{};
            } else {
                using NextList = decltype(filterTypesImpl<Map, Filter, WorldEntryDeclTs...>());
                return (typename NextList::template PrependT<typename Map<First>::type>){};
            }
        } else {
            if constexpr (sizeof...(WorldEntryDeclTs) == 0) {
                return TypeList<>{};
            } else {
                return decltype(filterTypesImpl<Map, Filter, WorldEntryDeclTs...>()){};
            }
        }
    }

    template<template<typename> typename Map, ConstraintType Filter, typename...WorldEntryDeclTs>
    auto filterTypes() {
        if constexpr(sizeof...(WorldEntryDeclTs) == 0) {
            return TypeList<>{};
        } else {
            return filterTypesImpl<Map, Filter, WorldEntryDeclTs...>();
        }
    }

    template<template<typename> typename Map, ConstraintType Filter, typename...WorldEntryDeclTs>
    using FilterTypeT = decltype(filterTypes<Map, Filter, WorldEntryDeclTs...>());

    template<typename T>
    struct ExtractEntryType;

    template<WorldEntryName Name, typename ValueT>
    struct ExtractEntryType<Provides<Name, ValueT>> {
        using type = WorldEntry<Name, ValueT>;
    };

    template<WorldEntryName Name, typename ValueT>
    struct ExtractEntryType<Requires<Name, ValueT>> {
        using type = WorldEntry<Name, ValueT>;
    };

    template<typename T>
    struct Identity {
        using type = T;
    };

    template<typename T>
    struct WorldUpdateFromProvides;

    template<typename...Ts>
    struct WorldUpdateFromProvides<TypeList<Ts...>> {
        using WorldUpdateT = WorldUpdate<Ts...>;
    };

    template<WorldEntryName Name, typename ValueT>
    struct RequiresPolicy<WorldEntry<Name, ValueT>> {
        template<typename EntryT>
        static constexpr bool allow() {
            return std::is_same_v<WorldEntry<Name, ValueT>, EntryT>;
        }
    };

    template<WorldEntryName Name>
    struct RequiresPolicy<RequiresNameOnly<Name>> {
        template<typename EntryT>
        static constexpr bool allow() {
            return EntryT::name == Name;
        }
    };
}

template<typename...WorldEntryDeclTs>
class TestStep {
public:
    using Provides = detail::FilterTypeT<detail::ExtractEntryType, ConstraintType::Provides, WorldEntryDeclTs...>;
    using Requires = detail::FilterTypeT<detail::ExtractEntryType, ConstraintType::Requires, WorldEntryDeclTs...>;
    using RequiresNameOnly = detail::FilterTypeT<detail::Identity, ConstraintType::RequiresNameOnly, WorldEntryDeclTs...>;

    using WorldUpdateT = typename detail::WorldUpdateFromProvides<Provides>::WorldUpdateT;

    template<typename...Ts>
    static WorldUpdateT worldUpdate(Ts&&...args) {
        return WorldUpdateT{std::forward<Ts>(args)...};
    }

    // derived classes need to implement

    // WorldUpdateT step(auto world) const;
    // void cleanup(auto world) const;
};

}
