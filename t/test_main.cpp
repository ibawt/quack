#include "gtest/gtest.h"
#include "engine.h"
#include "memory.h"
#include "quack.h"
#include "cps.h"
#include "types.h"
#include <string>
#include <vector>
#include <cstring>


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
  void run_test_case(const char *s, q_atom b) {
    q_atom a;

    auto r = q_parse_buffer(memory, s, strlen(s), &a);
    EXPECT_EQ(q_ok, r) << s;

    ASSERT_TRUE(q_equals(a, b)) << s;
  }

    q_atom make_string(const char *s) {
      q_atom a = {.pval = q_memory_alloc_string(memory, (char*)s, strlen(s))};
      return a;
    }

    q_atom make_list(std::vector<q_atom> vals) {
        q_cons *head = nullptr;
        assert(vals.size() > 0);
        for(int i = vals.size() - 1 ; i >= 0 ; --i) {
            head = q_memory_alloc_cons(memory, vals[i], head);
        }
        return q_atom_from_ptr(head);
    }
};

TEST_F(Tester, Equals) {
    struct testcase {
        q_atom a, b;
    };
    testcase cases[] = {
        { make_integer(5), make_integer(5)},
        { make_string("foo"), make_string("foo")},
        { make_list({ make_integer(5)}), make_list({make_integer(5)})},
        { make_list({ SYM("quote"), SYM("foo" )}), make_list({ SYM("quote"), SYM("foo")}) },
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

TEST_F(Tester, ParserTests) {
    testcase cases[] = {
        { "5", make_integer(5)},
        { "-5", make_integer(-5)},
        {"foo", make_symbol(q_symbol_create("foo"))},
        { "\"foo\"", make_string("foo") },
        { "'foo", make_list({ SYM("quote"), SYM("foo") } ) },
        { "foo", SYM("foo")}
    };

    for(auto& i : cases ) {
        run_test_case( i.in, i.expected);
    }
}

class CompilerTest : public testing::Test {
 protected:
  q_engine *engine;
 public:
  CompilerTest() {
    engine = q_engine_create();
  }
  ~CompilerTest() {
    q_engine_destroy(engine);
  }
};


