#!/usr/bin/env python3
"""
Reference only.

Conservative multi-depth controller distilled from the current experiments.
The production implementation should use the same ideas but should not depend
on pandas/numpy or on hidden-test information.
"""

from dataclasses import dataclass, field
from math import sqrt
from typing import List


@dataclass
class ArmStats:
    budget: int
    validation_means: List[float] = field(default_factory=list)

    @property
    def n(self) -> int:
        return len(self.validation_means)

    @property
    def mean(self) -> float:
        return sum(self.validation_means) / max(1, self.n)


@dataclass
class Candidate:
    seed: int
    arm_budget: int
    val_median: float
    val_mean: float
    val_dispersion: float
    active_nodes: int
    genome_id: str


class ConservativeDepthController:
    """
    All metrics are oriented so HIGHER = BETTER.
    For a loss such as MAE, pass -MAE.
    """

    def __init__(self, budgets=(200_000, 500_000, 1_000_000, 2_000_000),
                 beta=0.75, explore_every=5):
        self.arms = [ArmStats(b) for b in budgets]
        self.beta = beta
        self.explore_every = explore_every
        self.decision_count = 0
        self.global_candidate = None

    @staticmethod
    def better_candidate(a: Candidate, b: Candidate | None) -> bool:
        if b is None:
            return True
        if a.val_median != b.val_median:
            return a.val_median > b.val_median
        if a.val_mean != b.val_mean:
            return a.val_mean > b.val_mean
        if a.val_dispersion != b.val_dispersion:
            return a.val_dispersion < b.val_dispersion
        return a.active_nodes < b.active_nodes

    def observe(self, arm_index: int, candidate: Candidate):
        self.arms[arm_index].validation_means.append(candidate.val_mean)
        if self.better_candidate(candidate, self.global_candidate):
            # Production system: do NOT promote immediately.
            # Send through the fresh confirmation gate first.
            self.global_candidate = candidate

    def choose_arm(self) -> int:
        self.decision_count += 1

        # Bootstrap: every arm must be sampled.
        unsampled = [i for i, a in enumerate(self.arms) if a.n == 0]
        if unsampled:
            return unsampled[0]

        # Forced exploration.
        if self.decision_count % self.explore_every == 0:
            return min(range(len(self.arms)),
                       key=lambda i: (self.arms[i].n, self.arms[i].budget))

        all_scores = [x for a in self.arms for x in a.validation_means]
        mu = sum(all_scores) / len(all_scores)
        var = sum((x - mu) ** 2 for x in all_scores) / max(1, len(all_scores) - 1)
        scale = max(var ** 0.5, 1e-6)

        # Lower-confidence estimate: prefer reliable arms.
        def lcb(i: int) -> float:
            a = self.arms[i]
            return a.mean - self.beta * scale / sqrt(a.n)

        return max(range(len(self.arms)), key=lcb)


def validation_summary(values):
    vals = sorted(values)
    n = len(vals)
    median = vals[n // 2] if n % 2 else 0.5 * (vals[n // 2 - 1] + vals[n // 2])
    mean = sum(vals) / n
    dispersion = sum(abs(x - median) for x in vals) / n
    return median, mean, dispersion
