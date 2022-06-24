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
    bool wide_index;
} machine_t;

byte_t *parse_block(FILE *fp, uint32_t *block_size);

void set_local_variable(int index, word_t value);

int8_t get_byte_operand(uint16_t index);

uint16_t get_short_operand(uint16_t index);

void push_stack(word_t value);

word_t pop_stack(void);

#endif //MACHINE_H
