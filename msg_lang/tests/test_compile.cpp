#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <optional>

#include "msg_lang/module.h"
#include "msg_lang/parser.h"

using namespace pt::msg_lang;

class TestModule: public ::testing::Test {
public:
    TestModule() {

    }
protected:
    std::vector<SourceFile> files;

    void addFile(const std::string& content, const std::string& fileName = "") {
        files.push_back(SourceFile{.path = fileName, .content = content});
    }

    module::Module compile() {
        std::vector<AstFile> asts;
        for (const auto& file: files) {
            asts.push_back(parse(file));
        }

        return module::compile(asts);
    }
};

bool containsMember(const std::vector<module::MessageMember>& members, module::DataType type, const std::string& name) {
    for (const auto& member: members) {
        if (member.name == name && member.type == type) {
            return true;
        }
    }
    return false;
}

std::optional<size_t> memberIdx(const std::vector<module::MessageMember>& members, const std::string& name) {
    size_t i = 0;
    for (const auto& member: members) {
        if (member.name == name) {
            return i;
        }
        i++;
    }
    return std::nullopt;
}


struct inAscendingOrder {
    template<typename T, typename...Ts>
    bool operator()(T first, T second, Ts...ts) {
        if constexpr (sizeof...(Ts) == 0) {
            return first < second;
        } else {
            return first < second && (*this)(second, ts...);
        }
    }
};

TEST_F(TestModule, should_contain_defined_message) {
    addFile("event E{}");
    auto m = compile();

    ASSERT_TRUE(m.hasMessage(ItemName{"E"}));
}

TEST_F(TestModule, empty_message_should_have_no_members) {
    addFile("event E{}");
    auto m = compile();

    auto message = m.messageByName(ItemName{"E"});
    ASSERT_TRUE(message);
    ASSERT_TRUE((*message)->members.empty());
}


TEST_F(TestModule, should_contain_defined_messages) {
    addFile(R"#(
event E{}
event F{}
)#");
    auto m = compile();

    ASSERT_TRUE(m.hasMessage(ItemName{"E"}));
    ASSERT_TRUE(m.hasMessage(ItemName{"F"}));
}

TEST_F(TestModule, message_with_int_should_have_int_member) {
    addFile(R"#(
event E {
    int hello
})#");

    auto m = compile();

    auto message = m.messageByName(ItemName{"E"});
    ASSERT_TRUE(message);

    const auto& members = (*message)->members;
    ASSERT_FALSE(members.empty());
    ASSERT_PRED3(containsMember, members, module::BuiltinType::Int, "hello");
}

TEST_F(TestModule, event_has_no_expected_response) {
    addFile("event E{}");
    auto m = compile();

    auto message = m.messageByName(ItemName{"E"});
    ASSERT_TRUE(message);
    ASSERT_EQ((*message)->expectedResponse, std::nullopt);
}

TEST_F(TestModule, message_members_should_be_in_order) {
    addFile(R"#(
event E {
    int hello
    float h2lasd
    str srtd
})#");

    auto m = compile();

    auto message = m.messageByName(ItemName{"E"});
    ASSERT_TRUE(message);

    const auto& members = (*message)->members;

    auto hello = memberIdx(members, "hello");
    auto h2lasd = memberIdx(members, "h2lasd");
    auto srtd = memberIdx(members, "srtd");

    ASSERT_TRUE(hello);
    ASSERT_TRUE(h2lasd);
    ASSERT_TRUE(srtd);
    ASSERT_PRED3(inAscendingOrder{}, *hello, *h2lasd, *srtd);
}


TEST_F(TestModule, namespace_should_be_nullopt_if_not_set) {
    addFile(R"#(
event E {
    int hello
    float h2lasd
    str srtd
})#");

    auto m = compile();

    ASSERT_EQ(m.withNamespace, std::nullopt);
}

TEST_F(TestModule, namespace_can_be_set_with_namespace_keyword) {
    addFile(R"#(
namespace testNamespace

event E {
    int hello
    float h2lasd
    str srtd
})#");

    auto m = compile();
    ASSERT_EQ(m.withNamespace, "testNamespace");
}

TEST_F(TestModule, multiple_namespaces_set_should_be_an_error) {
    addFile(R"#(
namespace testNamespace
namespace testNamespaceConflict

event E {
    int hello
    float h2lasd
    str srtd
})#");

    auto m = compile();
    ASSERT_NE(m.errors, std::vector<Error>());
}


TEST_F(TestModule, multiple_namespaces_set_should_be_an_error_unless_there_is_no_conflict) {
    addFile(R"#(
namespace testNamespace
namespace testNamespace
)#");

    auto m = compile();
    ASSERT_EQ(m.withNamespace, "testNamespace");
    ASSERT_EQ(m.errors, std::vector<Error>());
}
