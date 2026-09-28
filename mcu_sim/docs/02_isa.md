# Bytecode ISA

The initial instruction set includes state and constant loads, local memory operations, register movement, arithmetic, min/max, absolute value, bitwise operations, shifts, comparison, select, and output.

Instructions use fixed-width fields and int32 values. The enum is intentionally small and easy to extend.

