#ifndef STACK_H
#define STACK_H

#include "ijvm.h"

typedef struct ijvm_stack {
    uint32_t current;
    word_t *entries;
    uint32_t size;
} ijvm_stack_t;

ijvm_stack_t *init_stack(uint32_t size);

void destroy_stack(ijvm_stack_t *stack);

word_t top_stack(ijvm_stack_t *stack);

word_t pop_stack(ijvm_stack_t *stack);

void push_stack(ijvm_stack_t *stack, word_t value);

uint32_t size_stack(ijvm_stack_t *stack);

#endif //STACK_H
