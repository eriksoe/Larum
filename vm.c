#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "vm.h"

/* Small VM modelled closely on the stack-based "Gullwing" CPU design by
   Charles Eric LaForest.
 */
#define OPCODE_BITS 5
#define OPCODE_MASK ((1 << OPCODE_BITS) - 1)

enum Opcode {
    /*---------- Special -----------------------------------*/
    FETCH_INS = 0,
    NOP,

    /*---------- Control -----------------------------------*/
    JMP,                        /* Jump unconditionally <address> */
    JMPZ,                       /* Jump if TOP is zero <address> */
    JMPN,                       /* Jump if TOP is negative <address> */
    CALL,                       /* Push PC->R, then jump <address> */
    RET,                        /* Pop R and jump to old top */

    /*---------- Stack manipulation ------------------------*/
    DUP,                        /* a b -> a */
    SWAP,                       /* a b -> b a */
    OVER,                       /* a b -> a b a */

    /*---------- Internal moves ----------------------------*/
    PUSH_A,
    POP_A,
    PUSH_R,
    POP_R,

    /*---------- Arithmetic --------------------------------*/
    ADD,
    SUB,
    MUL,

    /*---------- Bitwise operations ------------------------*/
    //----- Unary:
    SHR,                        /* Shift right arithmetically */
    SHL,                        /* Shift left */
    NOT,                        /* Bit negation (unary) */
    //----- Binary:
    AND,
    XOR,
    OR,

    /*---------- Fetch/store -------------------------------*/
    //----- By PC:
    LIT,                        /* Push literal <constant> */
    LIT_1,                      /* Push the literal number 1  */
    //----- By A:
    LOAD_A,
    STORE_A,
    LOAD_A_INC,                 /* Load with post-inc */
    STORE_A_INC,                /* Store with post-inc */
    //----- By R:
    LOAD_R_INC,                 /* Load with post-inc */
    STORE_R_INC,                /* Store with post-inc */

    /*---------- Special-operations ------------------------*/
    BUILTIN,                    /* Special, complex operation <op-addr> */

    OPCODE_COUNT
};


void vm_loop(Regs* regs) {
    /* Init from saved state: */
    Word* PC   = regs->PC;
    Word* TOSp = regs->TOSp;
    Word** Rp   = regs->Rp;
    Word A     = regs->A;

    /* Init other state: */
    uint32_t isr = 0;

#define READ_FROM_INS_STREAM() (*(PC++))
#define POP() (*(TOSp--))
#define PUSH(x) (*(++TOSp) = (x))
#define PEEK() (*TOSp)
#define STACK_AT(n) (*(TOSp - (n)))
#define REPLACE(x) (*TOSp = (x))
#define JUMP(addr) { PC =  (addr); isr = 0; }
    if (setjmp(regs->jmp_env)) {
        return;
    }
    while (1) {
        printf("DB| ISR before shift: %x\n", isr);
        int opcode = isr & OPCODE_MASK;
        printf("DB| TOS %d @ %p\n", PEEK(), TOSp);
        printf("DB| opcode: %d\n", opcode);
        isr >>= OPCODE_BITS;
        switch (opcode) {
            /*---------- Special -----------------------------------*/
        case FETCH_INS: {
            isr = READ_FROM_INS_STREAM();
            printf("DB| FETCH_INS => %x\n", isr);
        } break;
        case NOP: {
        } break;
    /*---------- Control -----------------------------------*/
        case JMP: {
            JUMP(regs->program_base + READ_FROM_INS_STREAM());
        } break;
        case JMPZ: {
            Word addr = READ_FROM_INS_STREAM();
            if (PEEK() == 0) {JUMP(regs->program_base + addr);}
        } break;
        case JMPN: {
            Word addr = READ_FROM_INS_STREAM();
            if (PEEK() < 0) {JUMP(regs->program_base + addr);}
        } break;
        case CALL: {
            Word addr = READ_FROM_INS_STREAM();
            *(++Rp) = PC;
            JUMP(regs->program_base + addr);
        } break;
        case RET: {
            JUMP(*(Rp--));
        } break;
    /*---------- Stack manipulation ------------------------*/
        case DUP: {
            Word tmp = PEEK();
            PUSH(tmp);
        } break;
        case SWAP: {
            Word tmp = PEEK();
            REPLACE(STACK_AT(1));
            STACK_AT(1) = tmp;
        } break;
        case OVER: {
            Word tmp = STACK_AT(1);
            PUSH(tmp);
        } break;
    /*---------- Internal moves ----------------------------*/
    /*---------- Arithmetic --------------------------------*/
        case ADD: {
            Word tmp = POP();
            REPLACE(PEEK() + tmp);
        } break;
        case SUB: {
            Word tmp = POP();
            REPLACE(PEEK() - tmp);
        } break;
        case MUL: {
            Word tmp = POP();
            REPLACE(PEEK() * tmp);
        } break;
    /*---------- Bitwise operations ------------------------*/
        case SHR: {
            REPLACE(PEEK() >> 1);
        } break;
        case SHL: {
            REPLACE(PEEK() << 1);
        } break;
        case NOT: {
            REPLACE(~PEEK());
        } break;

        case AND: {
            Word tmp = POP();
            REPLACE(PEEK() & tmp);
        } break;
        case OR: {
            Word tmp = POP();
            REPLACE(PEEK() | tmp);
        } break;
        case XOR: {
            Word tmp = POP();
            REPLACE(PEEK() ^ tmp);
        } break;
    /*---------- Fetch/store -------------------------------*/
    //----- By PC:
        case LIT: {
            PUSH(READ_FROM_INS_STREAM());
        } break;
        case LIT_1: {
            PUSH(1);
        } break;
    //----- By A:
        case LOAD_A: {
            PUSH(regs->heap_base[A]);
        } break;
        case STORE_A: {
            regs->heap_base[A] = POP();
        } break;
        case LOAD_A_INC: {
            PUSH(regs->heap_base[A++]);
        } break;
        case STORE_A_INC: {
            regs->heap_base[A++] = POP();
        } break;
    //----- By R:
        case LOAD_R_INC: {
            //PUSH((Word)(*Rp++));
        } break;
        case STORE_R_INC: {
            //(*Rp++) = POP();
        } break;
    /*---------- Special-operations ------------------------*/
        case BUILTIN: {
            int opID = READ_FROM_INS_STREAM();
            if (opID < BI_MIN_RANGE && opID >= BI_MAX_RANGE) {
                fprintf(stderr, "Bad operation ID: %d\n", opID);
                exit(1);
            }

            /* Save state: */
            regs->PC = PC;
            regs->TOSp = TOSp;
            regs->Rp = Rp;
            regs->A = A;

            (*LARUM_BUILT_INS[opID])(regs);

            /* Restore state: */
            PC   = regs->PC;
            TOSp = regs->TOSp;
            Rp   = regs->Rp;
            A     = regs->A;
        } break;
        } // opcode switch
#undef PUSH
#undef POP
    }// loop

exit_vm: {}
}

#define INSPACK(a,rest) ((a) | ((rest) << OPCODE_BITS))
#define ASM(a,b,c,d,e,f) INSPACK((a), INSPACK((b), INSPACK((c), INSPACK((d), INSPACK((e), (f) )))))

int main() {
    Word program[] =
        //{ASM(LIT_1, LIT_1, ADD, BUILTIN, NOP, NOP)};
        //{ASM(LIT_1, LIT, ADD, BUILTIN, NOP, NOP), 41};
    //{ASM(LIT, LIT, MUL, BUILTIN, NOP, NOP), 6, 7};
        //{ASM(LIT, DUP, ADD, BUILTIN, NOP, NOP), 7};
        //{ASM(LIT, LIT, SUB, NOP, NOP, NOP), 7,9, ASM(LIT, LIT, SWAP, SUB, BUILTIN, NOP), 7,9};
    //{ASM(LIT, LIT, OVER, BUILTIN, NOP, NOP), 7,9};
        {ASM(LIT, LIT, XOR, NOP, NOP, NOP), 3,9,
         ASM(LIT, LIT, AND, NOP, NOP, NOP), 3,9,
         ASM(LIT, LIT, OR, NOP, NOP, NOP), 3,9,
         ASM(BUILTIN, BI_EXIT, NOP, NOP, NOP, NOP)};
    Word* ret_stack[100];
    Word data_stack[100];
    Word heap[4096];
    Regs r = {
        .program_base = program,
        .heap_base = heap,
        .PC = program,
        .TOSp = data_stack-1,
        .Rp = ret_stack-1,
        .A = 0
    };
    vm_loop(&r);
    printf("Stack (height %d): \n", r.TOSp-data_stack+1);
    for (Word* p = r.TOSp; p >= data_stack; p--) {
        printf("\t%d\n", *p);
    }
    printf("Number of used opcodes: %d\n", OPCODE_COUNT);
}
