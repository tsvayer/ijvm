#include <stdlib.h>
#include "stack.h"
#include "ijvm.h"

ijvm_stack_t *init_stack(uint32_t size) {
    ijvm_stack_t *stack = malloc(sizeof(ijvm_stack_t));
    stack->current = -1;
    stack->entries = malloc(sizeof(word_t) * size);
    stack->size = size;
    return stack;
}

void destroy_stack(ijvm_stack_t *stack) {
    free(stack->entries);
    free(stack);
}

word_t top_stack(ijvm_stack_t *stack) {
    //TODO: check for boundaries
    return stack->entries[stack->current];
}

word_t pop_stack(ijvm_stack_t *stack) {
    //TODO: check for boundaries
    word_t value = stack->entries[stack->current];
    stack->current -= 1;
    return value;
}

void push_stack(ijvm_stack_t *stack, word_t value) {
    //TODO: check for boundaries
    stack->current += 1;
    stack->entries[stack->current] = value;
}

uint32_t size_stack(ijvm_stack_t *stack) {
    return stack->size;
}
