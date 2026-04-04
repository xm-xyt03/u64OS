#include <kernel/test.hpp>

extern "C"
{
#include <boot/tty.h>
}

namespace ktest
{
    // Static storage for registered test cases – no heap dependency
    static TestCase s_cases[KTEST_MAX_CASES];
    static base::size_t s_case_nr = 0;

    auto register_test(const char *suite, const char *name, void (*fn)(FailInfo *)) -> void
    {
        if (s_case_nr >= KTEST_MAX_CASES)
            return;

        s_cases[s_case_nr].suite = suite;
        s_cases[s_case_nr].name = name;
        s_cases[s_case_nr].fn = fn;
        s_case_nr++;
    }

    // ── Output helpers (no stdlib) ────────────────────────────────────────────

    static auto puts_dec(base::int32_t n) -> void
    {
        boot_puti(n);
    }

    static auto print_location(const char *file, base::int32_t line) -> void
    {
        boot_puts("(");
        boot_puts(file);
        boot_puts(":");
        puts_dec(line);
        boot_puts(")");
    }

    // ── Public API ────────────────────────────────────────────────────────────

    auto run_all() -> TestResult
    {
        TestResult result = {0, 0};

        boot_puts("\n[KTEST] Running ");
        puts_dec(static_cast<base::int32_t>(s_case_nr));
        boot_puts(" test(s)...\n");

        for (base::size_t i = 0; i < s_case_nr; i++)
        {
            const TestCase &tc = s_cases[i];

            boot_puts("[TEST]  ");
            boot_puts(tc.suite);
            boot_puts("::");
            boot_puts(tc.name);
            boot_puts(" ... ");

            FailInfo fi = {nullptr, 0, nullptr};
            tc.fn(&fi);

            if (fi.file == nullptr)
            {
                boot_puts("PASS\n");
                result.passed++;
            }
            else
            {
                boot_puts("FAIL: ");
                boot_puts(fi.expr);
                boot_puts(" ");
                print_location(fi.file, fi.line);
                boot_puts("\n");
                result.failed++;
            }
        }

        boot_puts("[KTEST] Result: ");
        puts_dec(result.passed);
        boot_puts(" passed, ");
        puts_dec(result.failed);
        boot_puts(" failed.\n\n");

        return result;
    }

} // namespace ktest
