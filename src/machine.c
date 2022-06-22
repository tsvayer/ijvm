#include <stdlib.h>
#include "machine.h"
#include "util.h"

#define STACK_SIZE 0x100

static uint32_t swap_word(uint32_t num) {
    return ((num >> 24) & 0xff) | ((num << 8) & 0xff0000) | ((num >> 8) & 0xff00) | ((num << 24) & 0xff000000);
}

program_t current_program;

byte_t *parse_block(FILE *fp, uint32_t *block_size) {
    uint32_t origin;
    // Read and ignore the origin
    fread(&origin, sizeof(uint32_t), 1, fp);
    // Read block size
    fread(block_size, sizeof(uint32_t), 1, fp);
    // Convert endianness
    *block_size = swap_word(*block_size);
    // Allocate memory for block data
    byte_t *data = malloc(sizeof(byte_t) * *block_size);
    // Read block data
    fread(data, sizeof(byte_t), *block_size, fp);
    return data;
}

int init_ijvm(char *binary_file) {
    // Open ijvm file for reading
    FILE *fp = fopen(binary_file, "rb");
    uint32_t header;
    // Read header
    fread(&header, sizeof(uint32_t), 1, fp);
    // Convert endianness
    header = swap_word(header);
    // Check that it is actually the ijvm file
    if (header != MAGIC_NUMBER) {
        // If it is not ijvm file close file and return error
        fclose(fp);
        return -1;
    }
    // Reset program counter
    current_program.program_counter = 0;
    current_program.stack = init_stack(STACK_SIZE);
    // Parse Constant Pool block
    current_program.constant_pool = parse_block(fp, &current_program.constant_pool_size);
    // Parse Text block
    current_program.text = parse_block(fp, &current_program.text_size);

    current_program.input = stdin;
    current_program.output = stdout;
    // Close file and return success
    fclose(fp);
    return 0;
}

void destroy_ijvm() {
    // Reset program counter
    current_program.program_counter = 0;
    // Free Constant Pool block memory
    free(current_program.constant_pool);
    // Reset Text block size
    current_program.text_size = 0;
    // Free Text block memory
    free(current_program.text);
    // Reset Constant Pool Size
    current_program.constant_pool_size = 0;
    // Destroy Stack
    destroy_stack(current_program.stack);
    current_program.stack = NULL;

}

void increment_program_counter() {
    current_program.program_counter += 1;
}

void run() {
    while (!finished()) {
        step();
    }
}

bool step(void) {
    switch (get_instruction()) {
        case OP_BIPUSH: {
            increment_program_counter();
            int8_t arg = (int8_t) get_text()[get_program_counter()];
            push_stack(current_program.stack, arg);
            log("BIPUSH %d\n", arg);
            break;
        }
        case OP_DUP:
            log("DUP\n");
            break;
        case OP_ERR:
            log("ERR\n");
            break;
        case OP_GOTO:
            log("GOTO\n");
            break;
        case OP_IADD: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 + arg2);
            log("IADD\n");
            break;
        }
        case OP_ISUB: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 - arg2);
            log("ISUB\n");
            break;
        }
        case OP_IAND: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 & arg2);
            log("IAND\n");
            break;
        }
        case OP_IOR: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 | arg2);
            log("IOR\n");
            break;
        }
        case OP_IFEQ:
            log("IFEQ\n");
            break;
        case OP_IFLT:
            log("IFLT\n");
            break;
        case OP_ICMPEQ:
            log("ICMPEQ\n");
            break;
        case OP_IINC:
            log("IINC\n");
            break;
        case OP_ILOAD:
            log("ILOAD\n");
            break;
        case OP_IN: {
            int input = getc(current_program.input);
            if (input == EOF)
                input = '0';
            push_stack(current_program.stack, input);
            log("IN\n");
            break;
        }
        case OP_INVOKEVIRTUAL:
            log("INVOKEVIRTUAL\n");
            break;
        case OP_IRETURN:
            log("IRETURN\n");
            break;
        case OP_ISTORE:
            log("ISTORE\n");
            break;
        case OP_LDC_W:
            log("LDC_W\n");
            break;
        case OP_NOP:
            log("NOP\n");
            break;
        case OP_OUT: {
            word_t arg = pop_stack(current_program.stack);
            putc(arg, current_program.output);
            log("OUT\n");
            break;
        }
        case OP_POP: {
            pop_stack(current_program.stack);
            log("POP\n");
            break;
        }
        case OP_SWAP: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg2);
            push_stack(current_program.stack, arg1);
            log("SWAP\n");
            break;
        }
        case OP_WIDE:
            log("WIDE\n");
            break;
        case OP_HALT: {
            current_program.halted = true;
            log("HALT\n");
            break;
        }
        default: {
            current_program.halted = true;
            log("Unknown Instruction\n");
            break;
        }

    }
    increment_program_counter();
    return true;
}

void set_input(FILE *fp) {
    current_program.input = fp;
}

void set_output(FILE *fp) {
    current_program.output = fp;
}

int get_program_counter(void) {
    return current_program.program_counter;
}

byte_t *get_text(void) {
    return current_program.text;
}

int text_size(void) {
    return current_program.text_size;
}

byte_t get_instruction(void) {
    return get_text()[get_program_counter()];
}

word_t tos(void) {
    return top_stack(current_program.stack);
}

int stack_size(void) {
    return size_stack(current_program.stack);
}

word_t *get_stack(void) {
    return current_program.stack->entries;
}

bool finished(void) {
    return current_program.halted
           || get_program_counter() >= text_size();
}
