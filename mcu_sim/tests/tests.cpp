#include <cgp_mcu/simulator.hpp>
#include <iostream>
#include <stdexcept>
using namespace cgp_mcu;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
  { FunctionalSimulator s(16); s.nodes = make_demo_nodes(); s.state[2]=3; s.state[10]=2; s.round(); require(s.state[3] == 1326, "demo program"); }
  // A two-node cycle proves all reads come from STATE(t), rather than partially published outputs.
  { FunctionalSimulator a(2), b(2); for (auto* s : {&a,&b}) { s->state={7,11}; s->nodes[0].program={{Op::LoadState,0,0,0,0,1},{Op::Output,0,0}}; s->nodes[1].program={{Op::LoadState,0,0,0,0,0},{Op::Output,0,0}}; }
    a.round(); b.round(true); require((a.state == State{11,7}) && a.state == b.state, "snapshot/order independence"); }
  // Private local RAM is only addressable within its owning Node object.
  { FunctionalSimulator s(2); s.nodes[0].local[0]=99; s.nodes[1].local[0]=3; s.nodes[1].program={{Op::LoadLocal,0,0,0,0,0},{Op::Output,0,0}}; s.round(); require(s.state[1]==3, "private RAM isolation"); }
  { FunctionalSimulator a(16), b(16); a.nodes=make_synthetic_nodes(16, 2); b.nodes=a.nodes; a.rounds(1000000); b.rounds(1000000); require(a.state==b.state, "one million deterministic rounds"); }
  { auto ns=make_synthetic_nodes(16, 16); FunctionalSimulator f(16); f.nodes=ns; f.rounds(1000); ParallelSimulator p(std::move(ns), State(16)); p.rounds(1000); require(f.state==p.state(), "parallel bit identity"); }
  { HardwareModel m; m.instructions_per_node=1; auto low=m.evaluate(); m.instructions_per_node=1024; auto high=m.evaluate(); require(low.communication_pct > high.communication_pct, "model trend"); }
  std::cout << "all tests passed\n";
}
