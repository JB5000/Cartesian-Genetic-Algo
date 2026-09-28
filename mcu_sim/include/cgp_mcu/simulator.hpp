#pragma once

#include <algorithm>
#include <array>
#include <barrier>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace cgp_mcu {

enum class Op : uint8_t { Nop, LoadState, LoadConst, LoadLocal, StoreLocal, Mov, Add, Sub, Mul,
                          Min, Max, Abs, And, Or, Xor, Shl, Shr, Cmp, Select, Output };
struct Instruction { Op op{Op::Nop}; uint8_t dst{}, a{}, b{}, c{}; int32_t imm{}; };
using State = std::vector<int32_t>;

inline int32_t wrap_add(int32_t x, int32_t y) { return static_cast<int32_t>(uint32_t(x) + uint32_t(y)); }
inline int32_t wrap_sub(int32_t x, int32_t y) { return static_cast<int32_t>(uint32_t(x) - uint32_t(y)); }
inline int32_t wrap_mul(int32_t x, int32_t y) { return static_cast<int32_t>(uint32_t(x) * uint32_t(y)); }

struct Node {
  std::array<int32_t, 16> regs{};
  std::vector<int32_t> local;
  std::vector<Instruction> program;
  int32_t output{};
  explicit Node(size_t local_words = 256) : local(local_words) {}

  int32_t execute(const State& snapshot) {
    for (const auto& i : program) {
      auto r = [&](uint8_t n) -> int32_t& { if (n >= regs.size()) throw std::out_of_range("register"); return regs[n]; };
      switch (i.op) {
        case Op::Nop: break;
        case Op::LoadState: r(i.dst) = snapshot.at(static_cast<size_t>(i.imm)); break;
        case Op::LoadConst: r(i.dst) = i.imm; break;
        case Op::LoadLocal: r(i.dst) = local.at(static_cast<size_t>(i.imm)); break;
        case Op::StoreLocal: local.at(static_cast<size_t>(i.imm)) = r(i.a); break;
        case Op::Mov: r(i.dst) = r(i.a); break;
        case Op::Add: r(i.dst) = wrap_add(r(i.a), r(i.b)); break;
        case Op::Sub: r(i.dst) = wrap_sub(r(i.a), r(i.b)); break;
        case Op::Mul: r(i.dst) = wrap_mul(r(i.a), r(i.b)); break;
        case Op::Min: r(i.dst) = std::min(r(i.a), r(i.b)); break;
        case Op::Max: r(i.dst) = std::max(r(i.a), r(i.b)); break;
        case Op::Abs: r(i.dst) = r(i.a) == std::numeric_limits<int32_t>::min() ? r(i.a) : -r(i.a); break;
        case Op::And: r(i.dst) = r(i.a) & r(i.b); break;
        case Op::Or: r(i.dst) = r(i.a) | r(i.b); break;
        case Op::Xor: r(i.dst) = r(i.a) ^ r(i.b); break;
        case Op::Shl: r(i.dst) = static_cast<int32_t>(uint32_t(r(i.a)) << (uint32_t(r(i.b)) & 31)); break;
        case Op::Shr: r(i.dst) = static_cast<int32_t>(uint32_t(r(i.a)) >> (uint32_t(r(i.b)) & 31)); break;
        case Op::Cmp: r(i.dst) = r(i.a) < r(i.b) ? -1 : (r(i.a) > r(i.b) ? 1 : 0); break;
        case Op::Select: r(i.dst) = r(i.a) ? r(i.b) : r(i.c); break;
        case Op::Output: output = r(i.a); break;
      }
    }
    return output;
  }
};

class FunctionalSimulator {
 public:
  std::vector<Node> nodes;
  State state;
  explicit FunctionalSimulator(size_t n = 16, size_t local_words = 256) : nodes(n, Node(local_words)), state(n) {}
  void round(bool reverse = false) {
    const State old = state; State next(nodes.size());
    for (size_t k = 0; k < nodes.size(); ++k) { const size_t i = reverse ? nodes.size() - 1 - k : k; next[i] = nodes[i].execute(old); }
    state.swap(next); // atomic publication at the simulator boundary
  }
  void rounds(size_t count) { while (count--) round(); }
};

class ParallelSimulator {
  std::vector<Node> nodes_; State state_, next_;
  size_t worker_count_;
  bool stopping_ = false;
  std::barrier<> start_, done_;
  std::vector<std::thread> workers_;
 public:
  explicit ParallelSimulator(std::vector<Node> nodes, State initial, size_t worker_count = 16)
      : nodes_(std::move(nodes)), state_(std::move(initial)), next_(nodes_.size()),
        worker_count_(std::min(worker_count, nodes_.size())),
        start_(static_cast<std::ptrdiff_t>(worker_count_ + 1)), done_(static_cast<std::ptrdiff_t>(worker_count_ + 1)) {
    if (nodes_.size() != state_.size()) throw std::invalid_argument("node/state count mismatch");
    if (!worker_count_) throw std::invalid_argument("worker count must be positive");
    for (size_t worker = 0; worker < worker_count_; ++worker) workers_.emplace_back([this, worker] {
      for (;;) { start_.arrive_and_wait(); if (stopping_) return;
        for (size_t i = worker; i < nodes_.size(); i += worker_count_) next_[i] = nodes_[i].execute(state_);
        done_.arrive_and_wait(); }
    });
  }
  ParallelSimulator(const ParallelSimulator&) = delete;
  ~ParallelSimulator() { stopping_ = true; start_.arrive_and_wait(); for (auto& t : workers_) t.join(); }
  void round() { start_.arrive_and_wait(); done_.arrive_and_wait(); state_.swap(next_); }
  void rounds(size_t count) { while (count--) round(); }
  const State& state() const { return state_; }
};

enum class Scenario { Optimistic, Realistic, Pessimistic };
struct TimingParameters { double gpio_us, barrier_us, turnaround_us, controller_us; };
inline TimingParameters timing(Scenario s) {
  if (s == Scenario::Optimistic) return {0.05, 0.10, 0.05, 0.10};
  if (s == Scenario::Pessimistic) return {0.80, 4.00, 1.00, 3.00};
  return {0.25, 1.00, 0.30, 0.75};
}
struct TimingResult { double compute_us, communication_us, sync_us, total_us, rounds_s, aggregate_ops_s, communication_pct; };
struct HardwareModel {
  size_t nodes = 16, instructions_per_node = 1; double worker_mhz = 144, bus_mhz = 10; unsigned state_bits = 32, bus_bits = 16;
  Scenario scenario = Scenario::Realistic; bool clustered = false; size_t cluster_size = 16;
  double cycles_per_instruction = 2.0;
  TimingResult evaluate() const {
    const auto p = timing(scenario); const double compute = instructions_per_node * cycles_per_instruction / worker_mhz;
    const double words = std::ceil(double(state_bits) / bus_bits);
    // A global bus serializes each node's publication. Clustered estimates concurrent local buses plus an inter-cluster exchange.
    double wire = nodes * words / bus_mhz;
    if (clustered && nodes > cluster_size) { const double clusters = std::ceil(double(nodes) / cluster_size); wire = cluster_size * words / bus_mhz + clusters * words / bus_mhz; }
    const double comm = wire + nodes * p.gpio_us + p.turnaround_us;
    const double sync = p.barrier_us + p.controller_us;
    const double total = compute + comm + sync, rps = 1e6 / total;
    return {compute, comm, sync, total, rps, rps * nodes * instructions_per_node, 100.0 * comm / total};
  }
};

inline std::vector<Node> make_synthetic_nodes(size_t n, size_t instructions) {
  std::vector<Node> out; out.reserve(n);
  for (size_t node = 0; node < n; ++node) { Node x; x.program.reserve(instructions + 1);
    for (size_t j = 0; j < instructions; ++j) {
      if (j % 4 == 0) x.program.push_back({Op::LoadState, uint8_t(j % 8), 0, 0, 0, int32_t((node + j) % n)});
      else x.program.push_back({j % 4 == 1 ? Op::Add : (j % 4 == 2 ? Op::Xor : Op::Mul), uint8_t(j % 8), uint8_t(j % 8), uint8_t((j + 1) % 8), 0, 0});
    }
    x.program.push_back({Op::Output, 0, uint8_t((instructions - 1) % 8), 0, 0, 0}); out.push_back(std::move(x));
  } return out;
}
inline std::vector<Node> make_demo_nodes() {
  auto nodes = make_synthetic_nodes(16, 2);
  nodes[3].program = {{Op::LoadState,0,0,0,0,10},{Op::LoadConst,1,0,0,0,221},{Op::Mul,2,0,1},{Op::LoadState,3,0,0,0,2},{Op::Mul,4,2,3},{Op::Output,0,4}};
  return nodes;
}
} // namespace cgp_mcu
