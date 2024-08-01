#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

// Useful macros
#define CLEAR_CHR(ARR) bzero(ARR, sizeof(ARR));

// General defines
#define EMULATOR_MEMORY_SIZE 4096
#define EMULATOR_REGISTERS_AMOUNT 16
#define EMULATOR_FONTSET_SIZE 80
#define EMULATOR_DISPLAY_WIDTH 64
#define EMULATOR_DISPLAY_HEIGHT 32
#define EMULATOR_STACK_SIZE 16
#define BYTE_SIZE 8
#define MALLOCATION_FALIED 1
#define EMULATOR_PC_STARTING_ADDR 0x200
#define EMULATOR_FILE_NAME_SIZE 40

// Flags defines
#define COLLISION_FLAG 0XF

// Commands defines
// Clears the display
#define OPCODE_CLEAR_DISPLAY 0x00E0

// Returns from a subroutine
#define OPCODE_RETURN_SUBROUTINE 0x00EE

// Jump to address NNN
#define OPCODE_JUMP_TO_ADDRESS 0x1000

// Set VX to NN
#define OPCODE_SET_REGISTER 0x6000

// ADD NN to v0
#define OPCODE_ADD_TO_REGISTER 0x7000

// Set index register I to the address NNN
#define OPCODE_SET_INDEX 0xA000

// Draw sprite
#define OPCODE_DRAW_SPRITE 0xD000

/*
// Emulator memory
unsigned char memory[4096];
* ===========
* - 512 bytes -
* 0x000
* interpeter
* 0x1FF
* - 256 bytes -
* 0x050
* font set
* 0x0A0
* - REST -
* ROM & RAM
* ===========
*/
/*
// Registers

// 16 8-bit registers
unsigned char V[16];

// Instruction pointer
unsigned short I;

// Program counter
unsigned short PC;

// Stack pointer
unsigned short sp;


// xLine & yLine

0110 1100
    x
 y |0 1 1 0
   |1 1 0 0
*/

typedef struct emulator_memory {
    unsigned char grid[EMULATOR_DISPLAY_HEIGHT * EMULATOR_DISPLAY_WIDTH];
    unsigned char memory[EMULATOR_MEMORY_SIZE];
    unsigned char v[EMULATOR_REGISTERS_AMOUNT];
    uint16_t stack[EMULATOR_STACK_SIZE];
    uint16_t opcode;
    uint16_t I;
    uint16_t pc;
    uint16_t sp;
} chip8_t, *pChip8;

// Fonts
unsigned char chip8_fontset[EMULATOR_FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80 // F

    // Additional letters

};


// Functions

// Load ROM - 1 sucsses, 0 fail
int load_rom(pChip8 chip8, char *filename) {
    // Open file
    FILE *rom = fopen(filename, "rb");
    if (rom == NULL) {
        perror("Failed to open file");
        return 1;
    }

    // Check the file size
    fseek(rom, 0, SEEK_END);
    long file_size = ftell(rom);
    rewind(rom);

    if (file_size > (EMULATOR_MEMORY_SIZE - EMULATOR_PC_STARTING_ADDR)) {
        perror("File is too big");
        fclose(rom);
        return 1;
    }

    fread(chip8->memory + EMULATOR_PC_STARTING_ADDR, sizeof(unsigned char), file_size, rom);

    // Close file
    fclose(rom);
    return 0;
}


// Init the chip8 structure
void chip8_init(const pChip8 chip8) {
    chip8->pc = EMULATOR_PC_STARTING_ADDR; // Program counter starts at 0x200
    chip8->opcode = 0; // Reset current opcode
    chip8->I = 0; // Reset index register
    chip8->sp = 0; // Reset stack pointer

    // Clear memory
    CLEAR_CHR(chip8->memory);

    // Clear registers
    CLEAR_CHR(chip8->v);

    // Clear stack
    CLEAR_CHR(chip8->stack);

    // Clear display
    CLEAR_CHR(chip8->grid);

    // Load font into memory
    for (int i = 0; i < EMULATOR_FONTSET_SIZE; ++i) {
        chip8->memory[i + 0x50] = chip8_fontset[i];
    }
}

void fetch_opcode(const pChip8 chip8) {
    // Retrieving the first byte, and shifting it 8 bits
    uint16_t high_byte = chip8->memory[chip8->pc] << 8;

    // Retrieving the first byte, and shifting it 8 bits
    uint16_t low_byte = chip8->memory[chip8->pc + 1];

    // Combining the bytes into word
    chip8->opcode = high_byte | low_byte;
}

void draw_sprite(pChip8 chip8, uint8_t xPos, uint8_t yPos, uint8_t height) {
    unsigned char pixels_row;
    chip8->v[COLLISION_FLAG] = 0; // Reset the collision flag

    for (int yLine = 0; yLine < height; yLine++) {
        pixels_row = chip8->memory[chip8->I + yLine];

        for (int xLine = 0; xLine < BYTE_SIZE; xLine++) {
            // Going thorugh each pixel in the row checking if it is on
            if ((pixels_row & (0x80 >> xLine)) != 0) {
                // Checks if length pass borders, if so we overlap from the other side
                unsigned int currX = (xPos + xLine) % EMULATOR_DISPLAY_WIDTH;
                unsigned int currY = (yPos + yLine) % EMULATOR_DISPLAY_HEIGHT;

                // Check collision
                if (chip8->grid[currX + (currY * EMULATOR_DISPLAY_WIDTH)] == 1)
                    chip8->v[COLLISION_FLAG] = 1;

                // Use XOR to determine the pixel
                chip8->grid[currX + (currY * EMULATOR_DISPLAY_WIDTH)] ^= 1;
            }
        }
    }
}


void execute_opcode(pChip8 chip8) {
    switch (chip8->opcode & 0xF000) {
        case 0x0000:
            switch (chip8->opcode & 0x00FF) {
                case OPCODE_CLEAR_DISPLAY: // 0x00E0
                    // Clear display
                    CLEAR_CHR(chip8->grid);
                    chip8->pc += 2;
                    break;
                case OPCODE_RETURN_SUBROUTINE: // 0x00EE
                    // Return from subroutine
                    chip8->sp--;
                    chip8->pc = chip8->stack[chip8->sp];
                    break;
                default:
                    printf("Uknown command : 0x0000");
                    chip8->pc += 2;
                    break;
            }
            break;

        case OPCODE_JUMP_TO_ADDRESS: // 0x1000
            // Jump to address NNN
            chip8->pc = chip8->opcode & 0x0FFF;
            break;

        case OPCODE_SET_REGISTER: // 0x6000
            // Set VX to NN
            chip8->v[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
            chip8->pc += 2;
            break;

        case OPCODE_ADD_TO_REGISTER: // 0x7000
            chip8->v[(chip8->opcode & 0x0F00) >> 8] += chip8->opcode & 0x00FF;
            chip8->pc += 2;
            break;

        case OPCODE_SET_INDEX: // 0xA000
            // Set I to address NNN
            chip8->I = chip8->opcode & 0x0FFF;
            chip8->pc += 2;
            break;

        case OPCODE_DRAW_SPRITE: // 0xD000
            // Draw sprite
            unsigned char x = chip8->v[(chip8->opcode & 0x0F00) >> 8];
            unsigned char y = chip8->v[(chip8->opcode & 0x00F0) >> 4];
            unsigned char height = chip8->opcode & 0x000F;
            draw_sprite(chip8, x, y, height);
            chip8->pc += 2;
            break;

        default:
            printf("Uknown Command or Command not suppoerted");
            chip8->pc += 2;
            break;
    }
}

void render_display(pChip8 chip8, GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_QUADS);
    for (int y = 0; y < EMULATOR_DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < EMULATOR_DISPLAY_WIDTH; ++x) {
            if (chip8->grid[x + y * EMULATOR_DISPLAY_WIDTH] == 1) {
                float x_pos = (x / (float)EMULATOR_DISPLAY_WIDTH) * 2.0f - 1.0f;
                float y_pos = 1.0f - (y / (float)EMULATOR_DISPLAY_HEIGHT) * 2.0f;

                float pixel_width = 2.0f / EMULATOR_DISPLAY_WIDTH;
                float pixel_height = 2.0f / EMULATOR_DISPLAY_HEIGHT;

                glVertex2f(x_pos, y_pos);  // top left
                glVertex2f(x_pos + pixel_width, y_pos); // top right
                glVertex2f(x_pos + pixel_width, y_pos - pixel_height); // bottom right
                glVertex2f(x_pos, y_pos - pixel_height); // bottom left
            }
        }
    }
    glEnd();

    glfwSwapBuffers(window);
}


/* Emulate cycle
 *===============
 * 1 ) Fetch opcode - Retrieve the next opcode from memory
 * 2 ) Decode and execute opcode - Execute the opcode
 * 3 ) Rended display - Render thedisplay if there any changes
 */
void emulate_cycle(pChip8 chip8, GLFWwindow *window) {
    // Fetch
    fetch_opcode(chip8);

    // Decode & Execute
    execute_opcode(chip8);
    /*chip8->I = 0x050;
    draw_sprite(chip8, 0, 0, 5);

    chip8->I = 0x050 + 5;
    draw_sprite(chip8, 5, 0, 5);*/
    // Render
    render_display(chip8, window);
}


int main() {
    pChip8 chip8 = (pChip8)malloc(sizeof(chip8_t));
    if (!chip8)
        exit(MALLOCATION_FALIED);
    char *filename = (char *)malloc(sizeof(char) * EMULATOR_FILE_NAME_SIZE);
    if (!filename)
        exit(MALLOCATION_FALIED);

    chip8_init(chip8);

    // Loading rom
    printf("Enter file name >>");
    fgets(filename, EMULATOR_FILE_NAME_SIZE, stdin);
    filename[strcspn(filename, "\n")] = 0;
    if (!glfwInit()) {
        perror("Failed init");
        return -1;
    }

    if (load_rom(chip8, filename) != 0) {
        perror("Failed to load rom");
        free(chip8);
        free(filename);
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(640, 320, "CHIP-8 Emulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    draw_sprite(chip8, 0, 0, 5); // Draw '0' at (0,0)

    // Set I to the location of the character '1' in the fontset
    chip8->I = 0x050 + 5; // 5 bytes per character
    draw_sprite(chip8, 5, 0, 5); // Draw '1' at (5,0)


    while(!glfwWindowShouldClose(window)) {
        // Emulation cycle
        emulate_cycle(chip8, window);

        // Window process
        glfwPollEvents();

    }

    for(int i = 0; i < EMULATOR_DISPLAY_HEIGHT; i ++) {
        printf("\n");
        for(int j = 0; j < EMULATOR_DISPLAY_WIDTH; j ++) {
            if (chip8->grid[j + i * EMULATOR_DISPLAY_WIDTH] == 1) {
                printf("*");
            }
            else printf(".");
        }
    }


    free(filename);
    free(chip8);
    return 0;
}
