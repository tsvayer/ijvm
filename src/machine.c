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

void increment_program_counter(int16_t offset) {
    current_program.program_counter += offset;
}

int8_t get_byte_operand() {
    return (int8_t) get_text()[get_program_counter() + 1];
}

int16_t get_short_operand() {
    byte_t operand1 = get_text()[get_program_counter() + 1];
    byte_t operand2 = get_text()[get_program_counter() + 2];
    return (int16_t) (operand1 * 0x100 + operand2);
}

void run() {
    while (!finished()) {
        step();
    }
}

bool step(void) {
    switch (get_instruction()) {
        case OP_BIPUSH: {
            int8_t arg = get_byte_operand();
            push_stack(current_program.stack, arg);
            increment_program_counter(2);
            log("BIPUSH %d\n", arg);
            break;
        }
        case OP_DUP: {
            word_t arg = top_stack(current_program.stack);
            push_stack(current_program.stack, arg);
            increment_program_counter(1);
            log("DUP\n");
            break;
        }
        case OP_ERR:
            increment_program_counter(1);
            log("ERR\n");
            break;
        case OP_GOTO: {
            int16_t offset = get_short_operand();
            log("GOTO %d\n", offset);
            increment_program_counter(offset);
            break;
        }
        case OP_IFEQ: {
            word_t arg = pop_stack(current_program.stack);
            int16_t offset = get_short_operand();
            if (arg != 0) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("IFEQ\n");
            break;
        }
        case OP_IFLT: {
            word_t arg = pop_stack(current_program.stack);
            int16_t offset = get_short_operand();
            if (arg >= 0) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("IFLT\n");
            break;
        }
        case OP_ICMPEQ: {
            word_t arg1 = pop_stack(current_program.stack);
            word_t arg2 = pop_stack(current_program.stack);
            int16_t offset = get_short_operand();
            if (arg1 != arg2) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("ICMPEQ\n");
            break;
        }
        case OP_IADD: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 + arg2);
            increment_program_counter(1);
            log("IADD\n");
            break;
        }
        case OP_ISUB: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 - arg2);
            increment_program_counter(1);
            log("ISUB\n");
            break;
        }
        case OP_IAND: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 & arg2);
            increment_program_counter(1);
            log("IAND\n");
            break;
        }
        case OP_IOR: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg1 | arg2);
            increment_program_counter(1);
            log("IOR\n");
            break;
        }
        case OP_IINC:
            increment_program_counter(1);
            log("IINC\n");
            break;
        case OP_ILOAD:
            increment_program_counter(1);
            log("ILOAD\n");
            break;
        case OP_IN: {
            int input = getc(current_program.input);
            if (input == EOF)
                input = '0';
            push_stack(current_program.stack, input);
            increment_program_counter(1);
            log("IN\n");
            break;
        }
        case OP_INVOKEVIRTUAL:
            increment_program_counter(1);
            log("INVOKEVIRTUAL\n");
            break;
        case OP_IRETURN:
            increment_program_counter(1);
            log("IRETURN\n");
            break;
        case OP_ISTORE:
            increment_program_counter(1);
            log("ISTORE\n");
            break;
        case OP_LDC_W:
            increment_program_counter(1);
            log("LDC_W\n");
            break;
        case OP_NOP:
            increment_program_counter(1);
            log("NOP\n");
            break;
        case OP_OUT: {
            word_t arg = pop_stack(current_program.stack);
            putc(arg, current_program.output);
            increment_program_counter(1);
            log("OUT\n");
            break;
        }
        case OP_POP: {
            pop_stack(current_program.stack);
            increment_program_counter(1);
            log("POP\n");
            break;
        }
        case OP_SWAP: {
            word_t arg2 = pop_stack(current_program.stack);
            word_t arg1 = pop_stack(current_program.stack);
            push_stack(current_program.stack, arg2);
            push_stack(current_program.stack, arg1);
            increment_program_counter(1);
            log("SWAP\n");
            break;
        }
        case OP_WIDE:
            increment_program_counter(1);
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
