# Generalist Evolutionary Input→Output System — Codex Bundle

Date: 2026-09-30

This bundle collects the current state of the evolutionary-network investigation and is intended to be handed directly to Codex for implementation.

## Goal

Build one general evolutionary engine:

    INPUTS -> evolved compact graph -> OUTPUTS

The engine must not know whether the problem is arithmetic, logic, classification, regression, sensor calibration, control, a game, or model distillation.

Problem-specific code is limited to:
- input/output shape,
- data/environment adapter,
- fitness/error definition,
- train/challenge/validation split.

Target deployment:
- worker MCU: CH32V203C8T6 class, ~20 KB RAM,
- tile: 32 worker MCUs + 1 controller MCU,
- one independent evolutionary run per worker.

## What is already supported by experiments

Core:
- fixed-slot CGP-like graph,
- compact 4-byte nodes,
- `(1+1)` search,
- in-place mutation,
- delta + rollback,
- active-node-biased mutations,
- adaptive heavy-tail mutation size,
- neutral drift in inactive nodes,
- counterexample mining,
- compactness pressure.

Generalization:
- keep a separate training champion and generalization champion,
- use 5 independent validation subsets,
- mean of 5 validations gave the best average ranking correlation,
- median of 5 is useful as a robust primary champion selector,
- recommended champion ordering:
  1. validation median,
  2. validation mean,
  3. smaller active graph.

Modules:
- `CALL(module, a, b)`,
- automatic function-preserving promotion of active subgraphs,
- rare explicit module reuse is experimental,
- aggressive forced reuse should remain disabled.

Controller:
- workers should not all run at the same depth,
- useful tested depths: 200k, 500k, 1M, 2M and longer exploratory runs,
- a conservative adaptive controller matched the best fixed depth median on the tested real datasets,
- selection must be based on validation, never hidden test.

## Important experimental conclusions

Do NOT reintroduce these as defaults without a new benchmark:
- full-genome copy for every child,
- large population inside one MCU,
- many children per MCU generation,
- fixed high per-token mutation probability,
- aggressive restart only from a stall counter,
- aggressive module reuse,
- assuming a single long run is always better than multiple independent runs.

## Directory map

- `SYSTEM_SPEC.md` — current architecture and rules.
- `CODEX_IMPLEMENTATION_PROMPT.md` — direct implementation task for Codex.
- `IMPLEMENTATION_PLAN.md` — proposed order of work.
- `docs/BENCHMARK_SUMMARY.md` — measured findings and caveats.
- `src/reference/` — experimental C++ implementations. These are references, not a single final production source tree.
- `src/mcu/` — MCU runtime reference.
- `tools/adaptive_controller_reference.py` — reference scheduler logic.
- `results/` — selected CSVs from the investigation.
- `figures/` — selected plots.
- `data/` — small reproducibility datasets used in current benchmarks.

## Recommended first Codex action

Read, in this order:
1. `CODEX_IMPLEMENTATION_PROMPT.md`
2. `SYSTEM_SPEC.md`
3. `docs/BENCHMARK_SUMMARY.md`
4. `src/reference/generalist_final_hybrid_benchmark.cpp`
5. `src/reference/auto_promote_modules.cpp`
6. `src/mcu/evo_cgp_mcu_runtime.h`

Then create a clean production implementation rather than trying to merge the experimental files line-by-line.
