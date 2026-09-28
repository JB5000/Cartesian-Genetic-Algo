# Bus model

The first-order global bus transfers ceil(state_bits / bus_bits) words per worker. GPIO, turnaround, barrier, and controller terms are explicit parameters rather than hidden constants.

