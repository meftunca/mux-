#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

uint64_t fibonacci(uint64_t number) {
  return number < 2 ? number : fibonacci(number - 1) + fibonacci(number - 2);
}
uint32_t factorial(uint32_t number) {
  return number <= 1 ? number : factorial(number - 1) * number;
}
TEST_CASE("Factorials are computed", "[factorial]") {
  REQUIRE(factorial(1) == 1);
  REQUIRE(factorial(2) == 2);
  REQUIRE(factorial(3) == 6);
  REQUIRE(factorial(10) == 3'628'800);
}
TEST_CASE("Benchmark Fibonacci", "[!benchmark]") {
  REQUIRE(fibonacci(5) == 5);

  REQUIRE(fibonacci(20) == 6'765);
  BENCHMARK("fibonacci 20") { return fibonacci(20); };

  REQUIRE(fibonacci(25) == 75'025);
  BENCHMARK("fibonacci 25") { return fibonacci(25); };
}