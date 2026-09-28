#include <cgp_mcu/simulator.hpp>
#include <filesystem>
#include <iostream>
using namespace cgp_mcu;
using Clock = std::chrono::steady_clock;

double cpu_rps(size_t instructions, size_t rounds, size_t threads) {
  auto nodes = make_synthetic_nodes(16, instructions); State initial(16);
  const auto start = Clock::now();
  if (threads > 1) { ParallelSimulator s(std::move(nodes), initial, threads); s.rounds(rounds); }
  else { FunctionalSimulator s(16); s.nodes = std::move(nodes); s.state = initial; s.rounds(rounds); }
  return rounds / std::chrono::duration<double>(Clock::now() - start).count();
}
int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "results/sweep.csv";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path()); std::ofstream f(path);
  f << "kind,architecture,nodes,instructions_per_node,worker_mhz,bus_mhz,state_bits,threads,compute_us,communication_us,sync_us,total_round_us,rounds_per_second,aggregate_ops_per_second,communication_percentage,speedup_vs_single\n";
  constexpr size_t sizes[] = {1,4,8,16,32,64,128,256,512,1024}; constexpr double buses[] = {2,5,10,20}; constexpr double clocks[] = {48,72,96,144};
  for (size_t ins : sizes) {
    const size_t rounds = std::max<size_t>(200, 200000 / ins); const double single = cpu_rps(ins, rounds, 1);
    f << std::fixed << std::setprecision(3);
    f << "cpu,host,16," << ins << ",0,0,32,1,0,0,0," << 1e6/single << ',' << single << ',' << single*16*ins << ",0,1\n";
    for (size_t threads : {2u,4u,8u,16u}) { const double parallel = cpu_rps(ins, rounds, threads);
      f << "cpu,host,16," << ins << ",0,0,32," << threads << ",0,0,0," << 1e6/parallel << ',' << parallel << ',' << parallel*16*ins << ",0," << parallel/single << "\n";
    }
    for (size_t node_count : {4u,8u,16u,32u,64u,128u,256u}) for (bool clustered : {false, true}) for (double bus : buses) for (double clock : clocks) for (unsigned bits : {16u,32u}) {
      HardwareModel m; m.nodes=node_count; m.clustered=clustered; m.instructions_per_node=ins; m.bus_mhz=bus; m.worker_mhz=clock; m.state_bits=bits;
      for (auto s : {Scenario::Optimistic, Scenario::Realistic, Scenario::Pessimistic}) { m.scenario=s; auto r=m.evaluate();
        f << "mcu_" << (s==Scenario::Optimistic?"optimistic":s==Scenario::Realistic?"realistic":"pessimistic") << ',' << (clustered ? "clustered16" : "global_bus") << ',' << node_count << ',' << ins << ',' << clock << ',' << bus << ',' << bits << ",16," << r.compute_us << ',' << r.communication_us << ',' << r.sync_us << ',' << r.total_us << ',' << r.rounds_s << ',' << r.aggregate_ops_s << ',' << r.communication_pct << ",0\n";
      }
    }
  }
  std::cout << "Wrote " << path << "\n";
}
