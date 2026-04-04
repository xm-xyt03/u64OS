#ifndef KERNEL_TEST_HPP
#define KERNEL_TEST_HPP

#include <kernel/base.hpp>
#include <u64OS/err.h>

extern "C" {
#include <boot/tty.h>
}

namespace ktest
{
    // Maximum number of test cases that can be registered
    inline constexpr base::size_t KTEST_MAX_CASES = 128;

    // Per-test failure context (set by EXPECT_* macros)
    struct FailInfo
    {
        const char *file;
        base::int32_t line;
        const char *expr;
    };

    // A single test case descriptor
    struct TestCase
    {
        const char *suite;
        const char *name;
        void (*fn)(FailInfo *);
    };

    // Register a test case (called by static constructors via KTEST macro)
    auto register_test(const char *suite, const char *name, void (*fn)(FailInfo *)) -> void;

    struct TestResult
    {
        base::int32_t passed;
        base::int32_t failed;
    };

    // Run all registered tests and return summary
    auto run_all() -> TestResult;

} // namespace ktest

// ─── Internal helpers ────────────────────────────────────────────────────────

// Concatenation helpers for unique symbol names
#define _KTEST_CAT2(a, b) a##b
#define _KTEST_CAT(a, b)  _KTEST_CAT2(a, b)

// The test function signature that KTEST generates
#define _KTEST_FN(suite, name) _KTEST_CAT(_ktest_fn_, _KTEST_CAT(suite, _KTEST_CAT(_, name)))

// Static-constructor registrar object
#define _KTEST_REG(suite, name) _KTEST_CAT(_ktest_reg_, _KTEST_CAT(suite, _KTEST_CAT(_, name)))

// ─── Public API ──────────────────────────────────────────────────────────────

/**
 * KTEST(SuiteName, TestName)
 *
 * Declares and registers a test case. Usage:
 *
 *   KTEST(MyModule, SomeFeature) {
 *       EXPECT_EQ(1 + 1, 2);
 *   }
 */
#define KTEST(suite, name)                                                          \
    static void _KTEST_FN(suite, name)(ktest::FailInfo *_ktest_fail_info);         \
    namespace {                                                                      \
        struct _KTEST_REG(suite, name) {                                            \
            _KTEST_REG(suite, name)() {                                             \
                ktest::register_test(#suite, #name, &_KTEST_FN(suite, name));      \
            }                                                                        \
        } _KTEST_CAT(_ktest_reg_inst_, _KTEST_CAT(suite, _KTEST_CAT(_, name)));   \
    }                                                                                \
    static void _KTEST_FN(suite, name)(ktest::FailInfo *_ktest_fail_info)

// ─── Assertion macros ─────────────────────────────────────────────────────────

#define _KTEST_FAIL(expr_str)                       \
    do {                                            \
        _ktest_fail_info->file = __FILE__;          \
        _ktest_fail_info->line = __LINE__;          \
        _ktest_fail_info->expr = expr_str;          \
        return;                                     \
    } while (0)

#define EXPECT_TRUE(expr)                           \
    do {                                            \
        if (!(expr))                                \
            _KTEST_FAIL(#expr " is false");         \
    } while (0)

#define EXPECT_FALSE(expr)                          \
    do {                                            \
        if ((expr))                                 \
            _KTEST_FAIL(#expr " is true");          \
    } while (0)

#define EXPECT_EQ(a, b)                             \
    do {                                            \
        if (!((a) == (b)))                          \
            _KTEST_FAIL(#a " != " #b);              \
    } while (0)

#define EXPECT_NE(a, b)                             \
    do {                                            \
        if (!((a) != (b)))                          \
            _KTEST_FAIL(#a " == " #b);              \
    } while (0)

#define EXPECT_NULL(ptr)                                    \
    do {                                                    \
        if (!IS_ERR_PTR(ptr))                               \
            _KTEST_FAIL(#ptr " is not an error pointer");  \
    } while (0)

#define EXPECT_NOT_NULL(ptr)                                \
    do {                                                    \
        if (IS_ERR_PTR(ptr))                                \
            _KTEST_FAIL(#ptr " is an error pointer");      \
    } while (0)

#define EXPECT_GE(a, b)                             \
    do {                                            \
        if (!((a) >= (b)))                          \
            _KTEST_FAIL(#a " < " #b);               \
    } while (0)

#define EXPECT_LE(a, b)                             \
    do {                                            \
        if (!((a) <= (b)))                          \
            _KTEST_FAIL(#a " > " #b);               \
    } while (0)

#endif // !KERNEL_TEST_HPP
