#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <coroutine>
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

struct ReturnObject {
  struct promise_type {
    ReturnObject get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_void() {}
  };
};

ReturnObject counter(int start, int stop) {
  for (int i = start; i <= stop; ++i) {
    co_yield i;
  }
}

TEST_CASE("Counter coroutine", "[!benchmark]") {
  {
    for (int i : counter(1, 10)) {
      REQUIRE(i > 0);
    }
  }
}