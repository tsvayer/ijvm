#include <stdlib.h>
#include "machine.h"
#include "util.h"

#define STACK_SIZE 0x1000

static uint32_t swap_word(uint32_t num) {
    return ((num >> 24) & 0xff) | ((num << 8) & 0xff0000) | ((num >> 8) & 0xff00) | ((num << 24) & 0xff000000);
}

machine_t machine;

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

void increment_program_counter(int16_t offset) {
    machine.pc += offset;
}

int8_t get_byte_operand() {
    return (int8_t) get_text()[get_program_counter() + 1];
}

uint16_t get_ushort_operand() {
    uint16_t operand1 = get_text()[get_program_counter() + 1];
    uint16_t operand2 = get_text()[get_program_counter() + 2];
    return operand1 * 0x100 + operand2;
}

int16_t get_short_operand() {
    return (int16_t) get_ushort_operand();
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

word_t top_stack(void) {
    return *machine.sp;
}

void run() {
    while (step());
}

bool step(void) {
    switch (get_instruction()) {
        case OP_BIPUSH: {
            int8_t arg = get_byte_operand();
            push_stack(arg);
            increment_program_counter(2);
            log("BIPUSH %d\n", arg);
            break;
        }
        case OP_DUP: {
            word_t arg = top_stack();
            push_stack(arg);
            increment_program_counter(1);
            log("DUP\n");
            break;
        }
        case OP_GOTO: {
            int16_t offset = get_short_operand();
            increment_program_counter(offset);
            log("GOTO %d\n", offset);
            break;
        }
        case OP_IFEQ: {
            word_t arg = pop_stack();
            int16_t offset = get_short_operand();
            if (arg != 0) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("IFEQ %d\n", offset);
            break;
        }
        case OP_IFLT: {
            word_t arg = pop_stack();
            int16_t offset = get_short_operand();
            if (arg >= 0) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("IFLT %d\n", offset);
            break;
        }
        case OP_ICMPEQ: {
            word_t arg1 = pop_stack();
            word_t arg2 = pop_stack();
            int16_t offset = get_short_operand();
            if (arg1 != arg2) {
                offset = 3;
            }
            increment_program_counter(offset);
            log("ICMPEQ %d\n", offset);
            break;
        }
        case OP_IADD: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 + arg2);
            increment_program_counter(1);
            log("IADD\n");
            break;
        }
        case OP_ISUB: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 - arg2);
            increment_program_counter(1);
            log("ISUB\n");
            break;
        }
        case OP_IAND: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 & arg2);
            increment_program_counter(1);
            log("IAND\n");
            break;
        }
        case OP_IOR: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg1 | arg2);
            increment_program_counter(1);
            log("IOR\n");
            break;
        }
        case OP_IN: {
            int input = getc(machine.input);
            if (input == EOF)
                input = '0';
            push_stack(input);
            increment_program_counter(1);
            log("IN\n");
            break;
        }
        case OP_LDC_W: {
            uint16_t index = get_ushort_operand();
            push_stack(get_constant(index));
            increment_program_counter(3);
            log("LDC_W %d\n", index);
            break;
        }
        case OP_ISTORE: {
            word_t local = pop_stack();
            byte_t index = get_byte_operand();
            set_local_variable(index, local);
            increment_program_counter(2);
            log("ISTORE %d\n", index);
            break;
        }
        case OP_ILOAD: {
            int8_t index = get_byte_operand();
            word_t local = get_local_variable(index);
            push_stack(local);
            increment_program_counter(2);
            log("ILOAD %d\n", index);
            break;
        }
        case OP_IINC: {
            uint16_t arg = get_ushort_operand();
            byte_t index = arg / 0x100;
            int8_t value = (int8_t) (arg % 0x100);
            set_local_variable(index, get_local_variable(index) + value);
            increment_program_counter(3);
            log("IINC %d %d\n", index, value);
            break;
        }
        case OP_NOP:
            increment_program_counter(1);
            log("NOP\n");
            break;
        case OP_OUT: {
            word_t arg = pop_stack();
            putc(arg, machine.output);
            increment_program_counter(1);
            log("OUT\n");
            break;
        }
        case OP_POP: {
            pop_stack();
            increment_program_counter(1);
            log("POP\n");
            break;
        }
        case OP_SWAP: {
            word_t arg2 = pop_stack();
            word_t arg1 = pop_stack();
            push_stack(arg2);
            push_stack(arg1);
            increment_program_counter(1);
            log("SWAP\n");
            break;
        }
        case OP_WIDE:
            //TODO:
            increment_program_counter(1);
            log("WIDE\n");
            break;
        case OP_INVOKEVIRTUAL:
            //TODO:
            increment_program_counter(1);
            log("INVOKEVIRTUAL\n");
            break;
        case OP_IRETURN:
            //TODO:
            increment_program_counter(1);
            log("IRETURN\n");
            break;
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
    return top_stack();
}

int stack_size(void) {
    return machine.sp - machine.stack;
}

word_t *get_stack(void) {
    return machine.stack;
}

bool finished(void) {
    return machine.halted
           || get_program_counter() >= text_size();
}

word_t get_constant(int i) {
    int offset = i * sizeof(word_t);
    return machine.cpp[offset] * 0x1000000
           + machine.cpp[offset + 1] * 0x10000
           + machine.cpp[offset + 2] * 0x100
           + machine.cpp[offset + 3];
}
