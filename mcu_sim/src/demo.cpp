#include <cgp_mcu/simulator.hpp>
#include <iostream>
using namespace cgp_mcu;
int main() {
  FunctionalSimulator sim(16); sim.nodes = make_demo_nodes(); sim.state[2] = 3; sim.state[10] = 2; sim.round();
  std::cout << "Node3 after one synchronous round: " << sim.state[3] << " (expected 1326)\n";
  HardwareModel m; m.instructions_per_node = 64; auto r = m.evaluate();
  std::cout << "Predicted 16-MCU realistic, 64 instructions/node: " << r.rounds_s << " rounds/s, " << r.communication_pct << "% communication\n";
}
