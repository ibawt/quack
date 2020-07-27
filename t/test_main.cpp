#include "gtest/gtest.h"
#include "quack.h"

static void run_test_case(const char* s, q_atom b)
{
    q_memory *mem = q_memory_create();
    q_atom a;

    auto r = q_parse_buffer(mem, s, strlen(s), &a);
    EXPECT_EQ(q_ok, r) << s;

    ASSERT_TRUE(q_equals(a, b)) << s;

    q_memory_destroy(mem);
}

struct testcase {
    const char *in;
    q_atom      expected;
};

class Tester : public ::testing::Test
{
    q_memory *memory;
protected:
    Tester() {
        memory = q_memory_create();
    }

    ~Tester() override {
        q_memory_destroy(memory);
    }
public:
    q_atom make_string(const char *s) {
        return (q_atom)q_memory_alloc_string(memory, (char*)s, strlen(s));
    }

    q_atom make_list(std::vector<q_atom> vals) {
        q_cons *head = nullptr;
        assert(vals.size() > 0);
        for(int i = vals.size() - 1 ; i >= 0 ; --i) {
            head = q_memory_alloc_cons(memory, vals[i], head);
        }
        return (q_atom)head;
    }
};

TEST_F(Tester, Equals) {
    struct testcase {
        q_atom a, b;
    };
    testcase cases[] = {
        { make_integer(5), make_integer(5)},
        { make_string("foo"), make_string("foo")},
        { make_list({ make_integer(5)}), make_list({make_integer(5)})}
    };
    testcase badcases[] = {
        { make_integer(4), make_integer(5)},
    };

    for(auto& i : cases) {
        ASSERT_TRUE(q_equals(i.a, i.b));
    }
    for(auto& i : badcases) {
        ASSERT_FALSE(q_equals(i.a, i.b));
    }
}

TEST(Parse, Simple) {
    testcase cases[] = {
        { "5", make_integer(5)},
        { "-5", make_integer(-5)},
        {"foo", make_symbol(q_symbol_create("foo"))},
    };

    for(auto& i : cases ) {
        run_test_case( i.in, i.expected);
    }
}
