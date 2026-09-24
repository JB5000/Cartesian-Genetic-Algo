#include <cassert>

#include "cga/genome.hpp"

int main() {
  cga::Genome genome;
  genome.input_count = 2;
  genome.nodes = {{cga::OpCode::Add, {0, 1}},
                  {cga::OpCode::Multiply, {2, 1}}};
  genome.outputs = {3};
  assert(genome.valid());
  const auto active = genome.active_nodes();
  assert(active.size() == 2);
  assert(active[0] == 0);
  assert(active[1] == 1);
  return 0;
}
