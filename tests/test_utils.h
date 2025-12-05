#pragma once

#include <cassert>
#include <iostream>
#include <string>

// Simple test framework macros - reusable across all test files
#define TEST(name) void name()

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_STREQ(a, b) assert(std::string(a) == std::string(b))

#define RUN_TEST(name)                                                         \
    do {                                                                       \
        std::cout << "Running " << #name << "... ";                            \
        name();                                                                \
        std::cout << "✓ PASSED" << std::endl;                                  \
    } while (0)
