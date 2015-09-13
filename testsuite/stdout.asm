#-*- mode: python -*-
# Stack begins as (ram_size, program_size).
# Store those:
CALL save_mem_limits
CALL build_string
CALL output_string
BUILTIN EXIT

:build_string
ADDR buf
>A
LIT 40
LIT 0
LIT1
DROP
:bs_loop

DUP
LIT 48
+
B!A+

# Increment:
LIT1
+

# Test for loop end:
OVER
OVER
-
JMPZ bs_loop_end
JMP bs_loop
:bs_loop_end
DROP
DROP
RET

:output_string
ADDR buf
LIT 40
BUILTIN WRITE_STDOUT
RET


:save_mem_limits # ( ram_size program_size -- )
ADDR heap_base
>A
!A

ADDR heap_max
>A
!A
RET


VAR heap_base
VAR heap_max
VAR buf(100)

