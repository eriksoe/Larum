#-*- mode: fundamental -*-
DROP
DROP
ADDR hello_string
CALL print_counted_string
BUILTIN EXIT

:print_counted_string
>A
B@A+ # Fetch length
# Do "A> SWAP":
>R
A>
R>
BUILTIN WRITE_STDOUT
RET


:hello_string
STRING "Hello!"
