#include <stdint.h>
#include <setjmp.h>

typedef int32_t Word;
struct Regs {
    Word* program_base;
    Word* heap_base;
    Word* PC;
    Word* TOSp;
    Word** Rp;
    Word A;
    /* TODO: stack limits. */

    jmp_buf jmp_env;
};

typedef void (*LarumBuiltIn)(Regs* vm_state);

extern LarumBuiltIn *LARUM_BUILT_INS;

// Builtins could (should?) be described with a code pointer. For now, however, we use an enum.
enum BuiltInIDs {
    BI_MIN_RANGE = 0,
    BI_EXIT = 0,
    BI_MAX_RANGE
};