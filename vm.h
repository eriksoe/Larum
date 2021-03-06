#include <stdint.h>
#include <setjmp.h>

typedef int32_t Word;
struct Regs {
    // Static:
    Word* mem;
    Word* data_stack;
    Word* return_stack;
    /* TODO: stack limits. */

    // Dynamis:
    Word* PC;
    Word* TOSp;
    Word* Rp;
    Word A;

    jmp_buf jmp_env;
};

typedef void (*LarumBuiltIn)(Regs* vm_state);

extern LarumBuiltIn *LARUM_BUILT_INS;

// Builtins could (should?) be described with a code pointer. For now, however, we use an enum.
enum BuiltInIDs {
    BI_MIN_RANGE = 0,
    BI_EXIT = 0,
    BI_HELLO,
    BI_DUMP_STACK,
    BI_WRITE_STDOUT,
    BI_MAX_RANGE
};
