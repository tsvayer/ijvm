#ifndef MACHINE_H
#define MACHINE_H

#include "ijvm.h"

typedef struct machine {
    byte_t *text;
    uint32_t text_size;
    byte_t *cpp; // Constant Pool Pointer
    uint32_t cp_size; // Constant Pool Size
    uint32_t pc; // Program Counter
    word_t *lv; // Local Variable Frame pointer
    word_t *stack;
    word_t *sp;
    FILE *input;
    FILE *output;
    bool halted;
} machine_t;

byte_t *parse_block(FILE *fp, uint32_t *block_size);

#endif //MACHINE_H
