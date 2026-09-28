# Recurrence

Cycles are valid because dependencies refer to the previous snapshot. For example, worker zero may read worker one while worker one reads worker zero; both read the values from the same prior round.

