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

static void LarumDumpStack(Regs* state) {
    Word *top = state->TOSp, *bottom = state->data_stack;
    printf("Stack (height %ld): \n", top - bottom + 1);
    for (Word* p = top; p >= bottom; p--) {
        printf("\t%d\n", *p);
    }
}

static void LarumWriteStdout(Regs* state) {
    printf("DB| In LarumWriteStdout\n");
    Word count = POP();
    Word addr = POP();
    printf("DB| In LarumWriteStdout count=%d addr=%x\n", count, addr);
    fwrite(state->mem + addr, 1, count, stdout);
}

LarumBuiltIn LARUM_BUILT_INSx[] = {
    [BI_EXIT] = LarumExit,
    [BI_HELLO] = LarumHello,
    [BI_DUMP_STACK] = LarumDumpStack,
    [BI_WRITE_STDOUT] = LarumWriteStdout
};
LarumBuiltIn* LARUM_BUILT_INS = LARUM_BUILT_INSx;
