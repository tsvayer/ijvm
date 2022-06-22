#ifndef MACHINE_H
#define MACHINE_H

#include "ijvm.h"
#include "stack.h"

typedef struct program {
    uint32_t program_counter;
    uint32_t text_size;
    byte_t *text;
    uint32_t constant_pool_size;
    byte_t *constant_pool;
    ijvm_stack_t *stack;
    FILE *input;
    FILE *output;
} program_t;

byte_t *parse_block(FILE *fp, uint32_t *block_size);

#endif //MACHINE_H
