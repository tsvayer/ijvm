#include <stdlib.h>
#include "machine.h"
#include "util.h"

#define STACK_SIZE 0x10000

machine_t machine;

static uint32_t swap_word(uint32_t num) {
    return ((num >> 24) & 0xff) | ((num << 8) & 0xff0000) | ((num >> 8) & 0xff00) | ((num << 24) & 0xff000000);
}

void run() {
    while (step());
}

bool step(void) {
    switch (get_instruction()) {
        case OP_BIPUSH: {
            word_t arg = get_byte_operand(1);
            push_stack(arg);
            machine.pc += 2;
            log("BIPUSH %d\n", arg);
            break;
        }
        case OP_DUP: {
            word_t arg = tos();
            push_stack(arg);
            machine.pc += 1;
            log("DUP\n");
            break;
        }
        case OP_GOTO: {
            int16_t offset = (int16_t)get_short_operand(1);
            machine.pc += offset;
            log("GOTO %d\n", offset);
            break;
        }
        case OP_IFEQ: {
            word_t arg = pop_stack();
            int16_t offset = (int16_t)get_short_operand(1);
            if (arg != 0) {
                offset = 3;
            }
            machine.pc += offset;
            log("IFEQ %d\n", offset);
            break;
        }
        case OP_IFLT: {
            word_t arg = pop_stack();
            int16_t offset = (int16_t)get_short_operand(1);
            if (arg >= 0) {
                offset = 3;
            }
            machine.pc += offset;
            log("IFLT %d\n", offset);
            break;
        }
        case OP_ICMPEQ: {
            word_t arg1 = pop_stack();
            word_t arg2 = pop_stack();
            word_t offset = get_short_operand(1);
            if (arg1 != arg2) {
                offset = 3;
            }
            machine.pc += offset;
            log("ICMPEQ %d\n", offset);
            break;
        }
        case OP_IADD: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 + arg2);
            machine.pc += 1;
            log("IADD\n");
            break;
        }
        case OP_ISUB: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 - arg2);
            machine.pc += 1;
            log("ISUB\n");
            break;
        }
        case OP_IAND: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 & arg2);
            machine.pc += 1;
            log("IAND\n");
            break;
        }
        case OP_IOR: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 | arg2);
            machine.pc += 1;
            log("IOR\n");
            break;
        }
        case OP_LDC_W: {
            word_t index = get_short_operand(1);
            push_stack(get_constant(index));
            machine.pc += 3;
            log("LDC_W %d\n", index);
            break;
        }
        case OP_ISTORE: {
            word_t local = pop_stack();
            word_t index = machine.wide_index
                           ? get_short_operand(1)
                           : get_byte_operand(1);
            set_local_variable(index, local);
            machine.pc += machine.wide_index ? 3 : 2;
            log("ISTORE %d\n", index);
            break;
        }
        case OP_ILOAD: {
            word_t index = machine.wide_index
                           ? get_short_operand(1)
                           : get_byte_operand(1);
            word_t local = get_local_variable(index);
            push_stack(local);
            machine.pc += machine.wide_index ? 3 : 2;
            log("ILOAD %d\n", index);
            break;
        }
        case OP_IINC: {
            word_t index = get_byte_operand(1);
            word_t value = get_byte_operand(2);
            set_local_variable(index, get_local_variable(index) + value);
            machine.pc += 3;
            log("IINC %d %d\n", index, value);
            break;
        }
        case OP_NOP:
            machine.pc += 1;
            log("NOP\n");
            break;
        case OP_IN: {
            int input = getc(machine.input);
            if (input == EOF)
                input = 0;
            push_stack(input);
            machine.pc += 1;
            log("IN\n");
            break;
        }
        case OP_OUT: {
            word_t arg = pop_stack();
            putc(arg, machine.output);
            machine.pc += 1;
            log("OUT\n");
            break;
        }
        case OP_POP: {
            pop_stack();
            machine.pc += 1;
            log("POP\n");
            break;
        }
        case OP_SWAP: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg2);
            push_stack(arg1);
            machine.pc += 1;
            log("SWAP\n");
            break;
        }
        case OP_WIDE: {
            machine.pc += 1;
            log("WIDE ");
            machine.wide_index = true;
            step();
            machine.wide_index = false;
            break;
        }
        case OP_INVOKEVIRTUAL: {
            // Keep current registers
            uint32_t prev_pc = machine.pc;
            word_t *prev_lv = machine.lv;
            // Load method pointer and set PC
            word_t method = get_short_operand(1);
            machine.pc = get_constant(method);
            // Read number of arguments and locals
            word_t num_args = get_short_operand(0);
            word_t num_locals = get_short_operand(2);
            // Set current frame base
            machine.lv = machine.sp - num_args + 1;
            // Make room for locals
            machine.sp += num_locals;
            // Store previous PC on stack
            push_stack(prev_pc);
            // Store pointer to PC in first argument (OBJREF)
            *machine.lv = machine.sp - machine.lv;
            // Store previous frame base
            push_stack(prev_lv - machine.stack);
            // Move to the next OP
            machine.pc += 4;
            log("INVOKEVIRTUAL %d\n", method);
            break;
        }
        case OP_IRETURN: {
            // Keep return value
            word_t return_value = pop_stack();
            // Get call registries
            word_t caller_pc = machine.lv[*machine.lv];
            word_t caller_lv = machine.lv[*machine.lv + 1];
            // Restore SP
            machine.sp = machine.lv;
            // Save return value on stack
            *machine.sp = return_value;
            // Restore caller registries
            machine.lv = machine.stack + caller_lv;
            machine.pc = caller_pc;
            // Move to the next OP
            machine.pc += 3;
            log("IRETURN\n");
            break;
        }
        case OP_ERR: {
            machine.halted = true;
            log("ERR\n");
            break;
        }
        case OP_HALT: {
            machine.halted = true;
            log("HALT\n");
            break;
        }
        default: {
            machine.halted = true;
            log("Unknown Instruction\n");
            break;
        }
    }
    return !finished();
}

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
    machine.halted = false;
    machine.wide_index = false;
    // Reset program counter
    machine.pc = 0;
    // Init stack
    machine.stack = malloc(sizeof(word_t) * STACK_SIZE);
    machine.lv = machine.stack;
    // Allow for 10 variables in the entry func
    machine.sp = machine.lv + 10;
    // Parse Constant Pool block
    machine.cpp = parse_block(fp, &machine.cp_size);
    // Parse Text block
    machine.text = parse_block(fp, &machine.text_size);
    // Initialize to standard I/O
    machine.input = stdin;
    machine.output = stdout;
    // Close file and return success
    fclose(fp);
    return 0;
}

void destroy_ijvm() {
    // Reset program counter
    machine.pc = 0;
    // Free Constant Pool block memory
    free(machine.cpp);
    // Reset Text block size
    machine.text_size = 0;
    // Free Text block memory
    free(machine.text);
    // Reset Constant Pool Size
    machine.cp_size = 0;
    // Destroy Stack
    free(machine.stack);
    machine.stack = NULL;
    machine.sp = NULL;
    machine.lv = NULL;
}

word_t get_local_variable(int i) {
    return machine.lv[i];
}

void set_local_variable(int index, word_t value) {
    machine.lv[index] = value;
}

int8_t get_byte_operand(uint16_t index) {
    return (int8_t) get_text()[get_program_counter() + index];
}

uint16_t get_short_operand(uint16_t index) {
    uint16_t operand1 = get_text()[get_program_counter() + index];
    uint16_t operand2 = get_text()[get_program_counter() + index + 1];
    return operand1 * 0x100 + operand2;
}

word_t get_constant(int i) {
    int offset = i * sizeof(word_t);
    return machine.cpp[offset] * 0x1000000
           + machine.cpp[offset + 1] * 0x10000
           + machine.cpp[offset + 2] * 0x100
           + machine.cpp[offset + 3];
}

void push_stack(word_t value) {
    machine.sp += 1;
    *machine.sp = value;
}

word_t pop_stack(void) {
    word_t value = *machine.sp;
    machine.sp -= 1;
    return value;
}

void set_input(FILE *fp) {
    machine.input = fp;
}

void set_output(FILE *fp) {
    machine.output = fp;
}

int get_program_counter(void) {
    return machine.pc;
}

byte_t *get_text(void) {
    return machine.text;
}

int text_size(void) {
    return machine.text_size;
}

byte_t get_instruction(void) {
    return get_text()[get_program_counter()];
}

word_t tos(void) {
    return *machine.sp;
}

int stack_size(void) {
    return machine.sp - machine.lv;
}

word_t *get_stack(void) {
    return machine.lv;
}

bool finished(void) {
    return machine.halted
           || get_program_counter() >= text_size();
}
