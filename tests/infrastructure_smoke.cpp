#include <catch2/catch_test_macros.hpp>

TEST_CASE("the C++20 verification harness executes", "[fast][infrastructure]")
{
    STATIC_REQUIRE(__cplusplus >= 202002L);
    SUCCEED("Catch2 discovered and executed the TDFA test binary");
}
