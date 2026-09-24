#include <chrono>
#include <iostream>
#include <random>

#include "cga/evaluate.hpp"
#include "cga/mutation.hpp"

int main() {
  std::mt19937 rng(7);
  const auto genome = cga::random_genome(8, 64, 1, rng);
  std::vector<float> inputs(8, 0.25F);
  constexpr std::size_t samples = 100000;
  volatile float sink = 0.0F;
  const auto begin = std::chrono::steady_clock::now();
  for (std::size_t index = 0; index < samples; ++index) {
    sink += cga::evaluate(genome, inputs).outputs.front();
  }
  const auto end = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(end - begin).count();
  std::cout << "samples=" << samples << " seconds=" << seconds
            << " evaluations_per_second=" << samples / seconds
            << " sink=" << sink << '\n';
}
