#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "vm.h"

/* Small VM modelled closely on the stack-based "Gullwing" CPU design by
   Charles Eric LaForest.
 */
#define OPCODE_BITS 5
#define OPCODE_MASK ((1 << OPCODE_BITS) - 1)

/* The magic number for the file is the UTF-8 # encoding of "L̼B",
 * where L is for "Larum" and "̼" is a seagull, and "B" is for "boot image".
 */
const uint32_t IMG_MAGIC = 0x42bccc4c; // For little-endian.

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
    DUP,                        /* a b -> a a */
    DROP,                       /* a b -> a */
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
        int opcode = isr & OPCODE_MASK;
        // printf("DB| TOS %d @ %p\n", PEEK(), TOSp);
        // printf("DB| opcode: %d\n", opcode);
        isr >>= OPCODE_BITS;
        switch (opcode) {
            /*---------- Special -----------------------------------*/
        case FETCH_INS: {
            isr = READ_FROM_INS_STREAM();
        } break;
        case NOP: {
        } break;
    /*---------- Control -----------------------------------*/
        case JMP: {
            Word addr = READ_FROM_INS_STREAM();
            JUMP(regs->mem + addr);
        } break;
        case JMPZ: {
            Word addr = READ_FROM_INS_STREAM();
            if (PEEK() == 0) {JUMP(regs->mem + addr);}
        } break;
        case JMPN: {
            Word addr = READ_FROM_INS_STREAM();
            if (PEEK() < 0) {JUMP(regs->mem + addr);}
        } break;
        case CALL: {
            Word addr = READ_FROM_INS_STREAM();
            *(++Rp) = PC;
            JUMP(regs->mem + addr);
        } break;
        case RET: {
            JUMP(*(Rp--));
        } break;
    /*---------- Stack manipulation ------------------------*/
        case DUP: {
            Word tmp = PEEK();
            PUSH(tmp);
        } break;
        case DROP: {
            POP();
        } break;
        /*
        case SWAP: {
            Word tmp = PEEK();
            REPLACE(STACK_AT(1));
            STACK_AT(1) = tmp;
            } break;
        */
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
            PUSH(regs->mem[A]);
        } break;
        case STORE_A: {
            regs->mem[A] = POP();
        } break;
        case LOAD_A_INC: {
            PUSH(regs->mem[A++]);
        } break;
        case STORE_A_INC: {
            regs->mem[A++] = POP();
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

}

#define INSPACK(a,rest) ((a) | ((rest) << OPCODE_BITS))
#define ASM(a,b,c,d,e,f) INSPACK((a), INSPACK((b), INSPACK((c), INSPACK((d), INSPACK((e), (f) )))))

static void error(const char* format, ...);
static void usage(const char* prgname);
static int load_boot_file(const char* bootfile, Word* mem, int ram_size);

int main(int argc, const char** argv) {
    if (argc != 2) {
        usage(argv[0]);
    }
    const char* bootfile = argv[1];

    /*
    Word program[] =
        //{ASM(LIT_1, LIT_1, ADD, BUILTIN, NOP, NOP)};
        //{ASM(LIT_1, LIT, ADD, BUILTIN, NOP, NOP), 41};
    //{ASM(LIT, LIT, MUL, BUILTIN, NOP, NOP), 6, 7};
        //{ASM(LIT, DUP, ADD, BUILTIN, NOP, NOP), 7};
        //{ASM(LIT, LIT, SUB, NOP, NOP, NOP), 7,9, ASM(LIT, LIT, SWAP, SUB, BUILTIN, NOP), 7,9};
    //{ASM(LIT, LIT, OVER, BUILTIN, NOP, NOP), 7,9};
    //    {ASM(LIT, LIT, XOR, NOP, NOP, NOP), 3,9,
    //     ASM(LIT, LIT, AND, NOP, NOP, NOP), 3,9,
    //     ASM(LIT, LIT, OR, NOP, NOP, NOP), 3,9,
    //     ASM(BUILTIN, NOP, NOP, NOP, NOP, NOP), BI_EXIT};
    //    {ASM(BUILTIN, BUILTIN, NOP, NOP, NOP, NOP),
    //     BI_HELLO, BI_EXIT};
    //    {ASM(BUILTIN, JMP, NOP, NOP, NOP, NOP), // Call "Hello" indefinitely.
    //     BI_HELLO, 0};
    {ASM(LIT, 0,0,0,0,0), 10, // Call "Hello" ten times.
     ASM(JMPZ, BUILTIN, LIT_1, SUB, JMP, 0),
     6, BI_HELLO, 1,
     ASM(BUILTIN, 0,0,0,0,0), //TODO: ought to pop.
     BI_EXIT};
    */

    Word* ret_stack[100];
    Word data_stack[100];
    int ram_size = 1*1024*1024;
    Word* mem = (Word*)malloc(ram_size);
    if (!mem) error("Could not allocate memory.");
    int program_size = load_boot_file(bootfile, mem, ram_size);
    fprintf(stderr, "Boot image loaded - size=%d\n", program_size);

    Word* init_PC = mem;

    // Push initial location info:
    data_stack[0] = ram_size;
    data_stack[1] = program_size;
    int init_sp = 1;

    Regs r = {
        .mem = mem,
        .PC = init_PC,
        .TOSp = data_stack+init_sp,
        .Rp = ret_stack-1,
        .A = 0
    };
    vm_loop(&r);
    printf("Stack (height %ld): \n", r.TOSp-data_stack+1);
    for (Word* p = r.TOSp; p >= data_stack; p--) {
        printf("\t%d\n", *p);
    }
}

static void usage(const char* prgname) {
    error("Usage: %s <bootfile>", prgname);
}

static int load_boot_file(const char* bootfile, Word* mem, int ram_size) {
    FILE* f = fopen(bootfile, "r");
    if (!f) error("Could not open boot file `%s': %s", bootfile, strerror(errno));
    int pos = 0;
    int left = ram_size;
    int nread;
    Word magic;
    Word chksum;
    if (fread(&magic, sizeof(Word), 1, f) != 1) goto read_error;
    if (magic != IMG_MAGIC) error("The file `%s' is not a boot file.", bootfile);
    if (fread(&chksum, sizeof(Word), 1, f) != 1) goto read_error;

    while ((nread = fread(&mem[pos], sizeof(Word), left, f)) != 0) {
        // fprintf(stderr, "Read: %d pos: %d left: %d\n", nread, pos, left);
        pos += nread; left -= nread;
    }
    if (ferror(f)) goto read_error;

    {
        Word sum = 0;
        for (int i=0; i<pos; i++) sum += mem[i];
        if (sum != chksum) error("The bootfile `%s' had a bad checksum and is unusable", bootfile);
    }

    fclose(f);
    return pos;

read_error:
    error("Boot file load failure: %s", strerror(errno));
    return 0;
}

static void error(const char* format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}
