# Snapshot semantics

A round copies the current public state into an immutable execution snapshot. Workers calculate into a next-state buffer. The next buffer is swapped only after every worker has completed, so no worker observes a partial update.

