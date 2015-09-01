#include "vm.h"
#include <stdio.h>

#define POP() (*(state->TOSp--))
#define PUSH(x) (*(++state->TOSp) = (x))
#define PEEK() (*state->TOSp)
#define STACK_AT(n) (*(state->TOSp - (n)))

static void LarumExit(Regs* state) {
    fprintf(stderr, "DB| In LarumExit\n");
    longjmp(state->jmp_env, 1);
}

static void LarumHello(Regs* state) {
    printf("Hello, World!\n");
}

LarumBuiltIn LARUM_BUILT_INSx[] = {
    [BI_EXIT] = LarumExit,
    [BI_HELLO] = LarumHello
};
LarumBuiltIn* LARUM_BUILT_INS = LARUM_BUILT_INSx;
