#include <stdlib.h>
#include <machine.h>

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
    if (header != MAGIC_NUMBER){
        // If it is not ijvm file close file and return error
        fclose(fp);
        return -1;
    }
    // Reset program counter
    current_program.program_counter = 0;
    // Parse Constant Pool block
    current_program.constant_pool = parse_block(fp, &current_program.constant_pool_size);
    // Parse Text block
    current_program.text = parse_block(fp, &current_program.text_size);
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
}

void run() {
    while(current_program.program_counter < current_program.text_size) {
        switch(current_program.text[current_program.program_counter]) {
            case OP_BIPUSH:
                printf("BIPUSH\n");
                break;
            case OP_DUP:
                printf("DUP\n");
                break;
            case OP_ERR:
                printf("ERR\n");
                break;
            case OP_GOTO:
                printf("GOTO\n");
                break;
            case OP_HALT:
                printf("HALT\n");
                break;
            case OP_IADD:
                printf("IADD\n");
                break;
            case OP_IAND:
                printf("IAND\n");
                break;
            case OP_IFEQ:
                printf("IFEQ\n");
                break;
            case OP_IFLT:
                printf("IFLT\n");
                break;
            case OP_ICMPEQ:
                printf("ICMPEQ\n");
                break;
            case OP_IINC:
                printf("IINC\n");
                break;
            case OP_ILOAD:
                printf("ILOAD\n");
                break;
            case OP_IN:
                printf("IN\n");
                break;
            case OP_INVOKEVIRTUAL:
                printf("INVOKEVIRTUAL\n");
                break;
            case OP_IOR:
                printf("IOR\n");
                break;
            case OP_IRETURN:
                printf("IRETURN\n");
                break;
            case OP_ISTORE:
                printf("ISTORE\n");
                break;
            case OP_ISUB:
                printf("ISUB\n");
                break;
            case OP_LDC_W:
                printf("LDC_W\n");
                break;
            case OP_NOP:
                printf("NOP\n");
                break;
            case OP_OUT:
                printf("OUT\n");
                break;
            case OP_POP:
                printf("POP\n");
                break;
            case OP_SWAP:
                printf("SWAP\n");
                break;
            case OP_WIDE:
                printf("WIDE\n");
                break;
        }
        current_program.program_counter += 1;
    }
    // TODO:
    // Step while you can
}

void set_input(FILE *fp) {
    // TODO: implement me
}

void set_output(FILE *fp) {
    // TODO: implement me
}

int get_program_counter() {
    return current_program.program_counter;
}

byte_t *get_text() {
    return current_program.text;
}

bool step(void) {
    // TODO:
    return true;
}

int text_size(void) {
    return current_program.text_size;
}

byte_t get_instruction(void) {
    // TODO:
    return 0;
}

word_t tos(void) {
    return 0;
}
