# Make a swap via memory:

LIT 123
LIT 456
# Stack is now (123 456)
ADDR a   # 456->a
>A
!A
ADDR b   # 123 -> b
>A
!A
ADDR a
>A
@A
ADDR b
>A
@A
# Stack is now (456 123)

BUILTIN EXIT
VAR a
VAR b(1)

