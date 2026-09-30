# Benchmark Summary

These are experimental findings, not claims of universal optimality.

## Search mechanics

- `(1+1)` per worker was a strong baseline and maps well to the MCU.
- Delta/rollback avoided child-genome copies and saved RAM.
- Uniform per-token mutation was not a good default.
- Active-biased + adaptive heavy-tail mutation was consistently more useful.
- Aggressive stall-based restart was worse than maintaining a portfolio of run depths.

## Boolean MUL3

A pure bit-level representation using only:
- AND
- OR
- XOR
- NOT

solved 3-bit × 3-bit multiplication in:
- 28 / 32 runs,
- 87.5% success within 5M evaluations/run.

This showed that representation can matter more than small mutation-rate tuning.

## Real datasets

The same general engine was tested on:
- Breast Cancer Wisconsin,
- Wine,
- Diabetes regression,
- Digits 8×8.

Performance varies strongly by problem and budget. These datasets are used as scaling/robustness tests, not to claim SOTA.

## Multi-output / Digits

A flat compact student hit a practical ceiling around the 60s in the experiments.

A shared-feature + output-head architecture produced a run around:
- 76.2% hidden-test accuracy.

This showed that modular/shared representation can raise the ceiling.

## Module promotion

Automatic module promotion improved typical robustness but did not automatically raise the ceiling.

Aggressive explicit reuse hurt.

Rare explicit reuse:
- small average effect,
- occasionally produced a much better run,
- therefore remains experimental.

## Five independent validation subsets

This was one of the strongest general improvements.

Average Spearman ranking correlation between validation and hidden test increased from roughly:
- previous single validation: ~0.51 average,
- five-validation mean: ~0.74 average.

Across 48 real runs, a protected generalization champion:
- beat the train-only champion in 34,
- lost in 10,
- tied in 4.

## Equal-compute depth vs restarts

With the improved five-validation selection and 2M total candidate evaluations:

- Breast Cancer:
  - 1×2M: 88.1%
  - 2×1M: 93.7%
  - 4×500k: 93.7%
  - 10×200k: 91.6%

- Wine:
  - all four tested allocations: ~93.3%

- Digits:
  - 1×2M: 36.9%
  - 2×1M: 36.2%
  - 4×500k: 50.4%
  - 10×200k: 37.6%

- Diabetes MAE (lower is better):
  - 1×2M: 62.4
  - 2×1M: 55.5
  - 4×500k: 55.5
  - 10×200k: 59.5

Conclusion:
- run depth matters,
- multiple independent runs often beat one long run,
- 500k–1M is currently a strong region but is not assumed universal.

## Adaptive controller

A conservative adaptive controller using the five-validation signal matched the median of the best fixed arm on all four real benchmarks in the current bootstrap study with the same 8M total candidate-evaluation budget.

Median results:
- Breast Cancer: adaptive 93.7%
- Wine: adaptive 93.3%
- Digits: adaptive 50.4%
- Diabetes: adaptive MAE 60.7

The result was stable for beta values 0.5, 0.75, and 1.0 in the current sensitivity test.

Important caveat:
- the controller comparison is partly bootstrap-based on measured evolutionary runs,
- a fully fresh end-to-end 32-worker experiment is still required before freezing firmware.

## Current bottleneck

The main remaining controller risk is selection bias:
- many workers produce many candidate champions,
- one may look unusually good on the fixed validations by chance.

Therefore the next required system feature is a fresh confirmation gate for global champion promotion.
