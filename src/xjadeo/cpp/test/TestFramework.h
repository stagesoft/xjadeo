#ifndef XJADEO_TESTFRAMEWORK_H
#define XJADEO_TESTFRAMEWORK_H

#include <string>
#include <vector>
#include <iostream>
#include <cassert>

namespace xjadeo {
namespace test {

/**
 * Simple test framework for xjadeo C++ components
 */
class TestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string message;
    };

    static TestFramework& instance() {
        static TestFramework inst;
        return inst;
    }

    void addTest(const std::string& name, bool (*testFunc)()) {
        tests_.push_back({name, testFunc});
    }

    int runAll() {
        std::cout << "Running " << tests_.size() << " test(s)...\n\n";
        
        int passed = 0;
        int failed = 0;

        for (const auto& test : tests_) {
            std::cout << "Test: " << test.name << " ... ";
            try {
                if (test.func()) {
                    std::cout << "PASSED\n";
                    passed++;
                } else {
                    std::cout << "FAILED\n";
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "FAILED (exception: " << e.what() << ")\n";
                failed++;
            } catch (...) {
                std::cout << "FAILED (unknown exception)\n";
                failed++;
            }
        }

        std::cout << "\n";
        std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
        
        return failed > 0 ? 1 : 0;
    }

private:
    struct Test {
        std::string name;
        bool (*func)();
    };
    
    std::vector<Test> tests_;
};

// Test assertion macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "Assertion failed: " << #a << " == " << #b << " (" << (a) << " != " << (b) << ") at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "Assertion failed: " << #a << " != " << #b << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

// Test registration macro
#define REGISTER_TEST(name) \
    static bool test_##name(); \
    namespace { \
        struct TestReg_##name { \
            TestReg_##name() { \
                xjadeo::test::TestFramework::instance().addTest(#name, test_##name); \
            } \
        }; \
        static TestReg_##name testReg_##name; \
    } \
    static bool test_##name()

} // namespace test
} // namespace xjadeo

#endif // XJADEO_TESTFRAMEWORK_H

