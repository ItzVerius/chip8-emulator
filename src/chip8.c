/*
 * Implementation of emulator
 */
#include "chip8.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#define INSTRUCTION_TABLE \
    X(0x0, op_0)    \
    X(0x1, op_1)    \
    X(0x2, op_2)    \
    X(0x3, op_3)    \
    X(0x4, op_4)    \
    X(0x5, op_5)    \
    X(0x6, op_6)    \
    X(0x7, op_7)    \
    X(0x8, op_8)    \
    X(0x9, op_9)    \
    X(0xA, op_A)    \
    X(0xB, op_B)    \
    X(0xC, op_C)    \
    X(0xD, op_D)    \
    X(0xE, op_E)    \
    X(0xF, op_F)

#define OP_LOW_BYTE(x)          ((x) & 0x00FF)
#define OP_NIBBLE(x)            ((x) & 0x000F)
#define OP_ADDR(x)              ((x) & 0x0FFF)
#define OP_X(x)                 (((x) >> 8) & 0x000F)
#define OP_Y(x)                 (((x) >> 4) & 0x000F)

#define STACKSIZE 16
#define KILOBYTES(x)    ((x)*1024)
#define BYTES(x)        ((x)*8)
#define MEMORYSIZE      KILOBYTES(4)
#define CHIP8_REGCOUNT  16
#define CHIP8_KEYCOUNT  16
#define FONTSET_START   0x50
#define PROGRAM_START   0x200
#define PIXEL_ON        0xFFFFFFFF
#define PIXEL_OFF       0x000000FF

const uint8_t chip8_fontset[80] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// System representation
struct Chip8{
    bool crashed;                           // Flag active in case of reading unkown instruction.
    uint8_t memory[MEMORYSIZE];             // RAM available (4 KB)
    uint8_t V[CHIP8_REGCOUNT];              // 8 bit general purpose registers (except VF, used as a flag by some instructions)
    uint16_t I;                             // Register for memory addresses (only 12 lower bits used)
    uint16_t pc;                            // Program counter
    uint8_t delay_timer;                    // Timer that if not 0, decrement at a 60Hz rate
    uint8_t sound_timer;                    // Timer that if not 0, decrement at a 60Hz rate
    uint16_t stack[STACKSIZE];              // Stack for return adresses when calling subroutines
    uint8_t sp;                             // Stack pointer
    uint64_t screen[CHIP8_PIXELCOUNT/64];   // 64 * 32 pixel screen display
    uint16_t keypad;                        // 4 * 4 keypad
    uint16_t prev_keypad;                   // Snapshot of last set of active keys
    bool draw_flag;                         // Active if there's a change in screen
    bool sound_play;                        // Active when a sound is needed to be played (here its just a single beep)
};

typedef void (*Chip8Instruction)(Chip8 *chip, uint16_t opcode);

#define X(index, func) static void func(Chip8 *chip, uint16_t opcode);
INSTRUCTION_TABLE
#undef X

#define X(code, op) [code] = op,
static const Chip8Instruction instruction_table[16] = {
    INSTRUCTION_TABLE
};
#undef X

// Obtain value stored in Vn
static inline uint8_t get_V(Chip8 *chip, size_t n){
    return chip->V[n];
}

// Store value in Vn
static inline void set_V(Chip8 *chip, size_t n, uint8_t value){
    chip->V[n] = value;
}

// Read n-th byte in memory
static inline uint8_t get_mem_byte(Chip8 *chip, size_t n){
    return chip->memory[n];
}

// Get a pointer to n-th byte in memory
static inline uint8_t *get_mem_pointer(Chip8 *chip, size_t n){
    return &chip->memory[n];
}

// Set the n-th byte in memory to a value
static inline void set_mem_byte(Chip8 *chip, size_t n, uint8_t value){
    chip->memory[n] = value;
}

// Insert to stack head
static inline void insert_to_stack(Chip8 *chip, uint16_t addr){
    chip->stack[chip->sp++] = addr;
}

// Pop last adress stored in the stack
static inline uint16_t pop_from_stack(Chip8 *chip){
    assert(chip->sp > 0);
    if (chip->sp < STACKSIZE){
        chip->stack[chip->sp] = 0;
    }
    return chip->stack[--chip->sp];
}

// Set pc to a value
static inline void pc_set(Chip8 *chip, uint16_t new_pc){
    chip->pc = new_pc;
}

// Increment pc to next instruction
static inline void pc_incr(Chip8 *chip){
    chip->pc += 2;
}

Chip8* chip8_init(){
    Chip8* chip = calloc(1, sizeof(Chip8));
    if(!chip) return NULL;
    chip->crashed = false;
    pc_set(chip, PROGRAM_START);
    memcpy(get_mem_pointer(chip, FONTSET_START), chip8_fontset, sizeof(chip8_fontset));
    chip->keypad = chip->prev_keypad = 0x0000;
    chip->draw_flag = false;
    return chip;
}

bool chip8_crashed(Chip8 *chip){
    return chip->crashed;
}

bool chip8_get_pixelstate(Chip8 *chip, size_t n){
    uint64_t row = chip->screen[n / 64];

    // Right shift until reaching desired bit, and removing the rest to the left
    return (bool)((row >> (CHIP8_WIDTH-1 - (n % 64))) & 0x01);
}

void chip8_set_pixelstate(Chip8 *chip, size_t i, bool value){
    assert(i < CHIP8_PIXELCOUNT);

    // Get which row the pixel is in
    size_t row = i / 64;

    // Get its position
    size_t pos = CHIP8_WIDTH-1 - (i % CHIP8_WIDTH);

    // Select said bit
    uint64_t mask = (uint64_t)1 << pos;

    // Make it 0
    chip->screen[row] &= ~(mask);

    if(value){
        // If pixel must be 1, insert it.
        chip->screen[row] |= mask;
    }
}

bool chip8_get_drawflag(Chip8 *chip){
    return chip->draw_flag;
}

bool chip8_get_soundflag(Chip8 *chip){
    return chip->sound_play;
}

void chip8_end_draw(Chip8 *chip){
    chip->draw_flag = false;
}

void chip8_notify_keypad_state(Chip8 *chip, size_t i, bool ispressed){
    assert(i < CHIP8_KEYCOUNT);

    // Save actual keypad state as previous one
    chip->prev_keypad = chip->keypad;

    // Calculate which bit starting from the end is the one we want
    size_t pos = CHIP8_KEYCOUNT-1 - i;

    // Select said bit
    uint16_t mask = (uint16_t)1 << pos;

    // Make it 0
    chip->keypad &= ~(mask);

    if(ispressed){
        // If key is pressed, insert 1
        chip->keypad |= mask;
    }
}

void chip8_update_timers(Chip8 *chip) {
    if (chip->delay_timer > 0) {
        chip->delay_timer--;
    }
    if (chip->sound_timer > 0) {
        chip->sound_timer--;
        chip->sound_play = true;
    } else {
        chip->sound_play = false;
    }
}

void chip8_load(Chip8 *chip, const uint8_t *program, size_t size){
  assert(size <= (KILOBYTES(4) - PROGRAM_START) && chip);
  memcpy(get_mem_pointer(chip, PROGRAM_START), program, size);
}

void chip8_cycle(Chip8 *chip){
    assert(chip);
    if (chip->crashed) return;
    /* Fetch instruction from memory */
    uint16_t opcode = get_mem_byte(chip, chip->pc);
    opcode = opcode << 8 | get_mem_byte(chip, chip->pc + 1);
    pc_incr(chip);
    /* Execute instruction */
    uint8_t type = opcode >> 12;
    instruction_table[type](chip, opcode);
}

void dump_chip_status(FILE *file, Chip8 *chip){
    assert(file && chip);
    fprintf(file, "\nMemory:\nReserved space 0x000 to 0x1FF:\n\n");
    for(size_t i = 0; i < PROGRAM_START; i++){
        // Address of start of line
        if(i % 16 == 0) {
			fprintf(file, "%04zX: ", i);
		}

        // Each byte
        fprintf(file, "%02X ", get_mem_byte(chip, i));

        // Newline if printed 16 bytes
        if((i + 1) % 16 == 0) {
			fprintf(file, "\n");
		}
    }

    fprintf(file, "\nRest of memory 0x200 to 0xFFF:\n\n");
    for(size_t i = 0x200; i < MEMORYSIZE; i++){
        // Address of start of line
        if(i % 16 == 0) {
			fprintf(file, "%04zX: ", i);
		}

        // Each byte
        fprintf(file, "%02X ", get_mem_byte(chip, i));

        // Newline if printed 16 bytes
        if((i + 1) % 16 == 0) {
			fprintf(file, "\n");
		}
    }

    fprintf(file, "\nRegister values:\n\n");

    for(size_t i = 0; i < CHIP8_REGCOUNT; i++){
        fprintf(file, "V%zX: %02X \t", i, get_V(chip, i));
        if((i+1) % 4 == 0){
            fprintf(file, "\n");
        }
    }

    fprintf(file, "\nI register: %03X\nPC: %03X\nSP: %02X\nDelay timer: %02X\nSound timer: %02X\n",
        chip->I, chip->pc, chip->sp, chip->delay_timer, chip->sound_timer);

    fprintf(file, "Stack: \n");
    for(size_t i = 0; i < chip->sp; i++){
        // Adress of start of line
        if(i % 8 == 0){
            fprintf(file, "%02zX: ", i);
        }

        fprintf(file, "%04X ", chip->stack[i]);

        // Newline if printed 8 bytes
        if((i+1) % 8 == 0){
            fprintf(file, "\n");
        }
    }
}

void chip8_destroy(Chip8 *chip) {
    free(chip);
}

/**
 * In case we find an unknown opcode, for debugging purposes.
 */
static void op_unknown(Chip8 *chip, uint16_t opcode){
    chip->crashed = true;
    fprintf(stderr, "\n[ERROR] Unknown opcode: 0x%04X at PC: 0x%04X\n",
        opcode, chip->pc - 2);
    char opt;
    do{
        fprintf(stderr, "Want to dump system information? [Y/N]\t");
        scanf(" %c", &opt);
        opt = toupper(opt);
    }while(opt != 'Y' && opt != 'N');
    if(opt == 'Y'){
        char dumpfile_name[128] = {0};
        fprintf(stderr, "\nInsert name of file to dump into: (leave empty for stderr)\t");
        scanf(" %s", dumpfile_name);
        if(strcmp(dumpfile_name, "") == 0){
            dump_chip_status(stderr, chip);
        } else {
            FILE *dumpfile = fopen(dumpfile_name, "w");
            if (!dumpfile){
                perror("Error creating file to dump into.");
                dump_chip_status(stderr, chip);
                exit(EXIT_FAILURE);
            }
            dump_chip_status(dumpfile, chip);
        }
    }
    exit(EXIT_FAILURE);
}

/**
 * Processing each type of instruction
 */

/**
 * 00E0 / 00EE - Display / Return
 * --------------------------------------------------
 * 00E0: CLS         -> Clear the display.
 * 00EE: RET         -> Return from subroutine.
 */
static void op_0(Chip8 *chip, uint16_t opcode) {
	switch(opcode){
	    case(0x00E0):
			for(size_t i = 0; i < sizeof(chip->screen)/sizeof(chip->screen[0]); i++){
			    chip->screen[i] = 0;
			};
			break;

		case(0x00EE):
		    pc_set(chip, pop_from_stack(chip));
		    break;

		default:
		    op_unknown(chip, opcode);
	}
}

/**
 * 1NNN - JP addr
 * --------------------------------------------------
 * Jump to location NNN
 */
static void op_1(Chip8 *chip, uint16_t opcode) {
	pc_set(chip, OP_ADDR(opcode));
}

/**
 * 2NNN - CALL addr
 * --------------------------------------------------
 * Call subroutine at NNN
 */
static void op_2(Chip8 *chip, uint16_t opcode) {
	insert_to_stack(chip, chip->pc);
	pc_set(chip, OP_ADDR(opcode));
}

/**
 * 3XNN - SE Vx, byte
 * --------------------------------------------------
 * Skip next instruction if Vx == NN:
 */
static void op_3(Chip8 *chip, uint16_t opcode) {
	uint8_t nn = OP_LOW_BYTE(opcode);
	uint8_t Vx = OP_X(opcode);
	if (nn == Vx){
	    pc_incr(chip);
	}
}

/**
 * 4XNN - SNE Vx, byte
 * --------------------------------------------------
 * Skip next instruction if Vx != NN:
 */
static void op_4(Chip8 *chip, uint16_t opcode) {
    uint8_t nn = OP_LOW_BYTE(opcode);
	uint8_t Vx = OP_X(opcode);
	if (nn != Vx){
	    pc_incr(chip);
	}
}

/**
 * 5XY0 - SE Vx, Vy
 * --------------------------------------------------
 * Skip next instruction if Vx == Vy:
 */
static void op_5(Chip8 *chip, uint16_t opcode) {
    uint8_t Vy = OP_Y(opcode);
	uint8_t Vx = OP_X(opcode);
	if (Vy == Vx){
	    pc_incr(chip);
	}
}

/**
 * 6XNN - LD Vx, byte
 * --------------------------------------------------
 * Set Vx = NN
 */
static void op_6(Chip8 *chip, uint16_t opcode) {
    set_V(chip, OP_X(opcode), OP_LOW_BYTE(opcode));
}

/**
 * 7XNN - ADD Vx, byte
 * --------------------------------------------------
 * Set Vx = Vx + NN (lowest 8 bits kept).
 * Note: VF carry flag is NOT modified.
 */
static void op_7(Chip8 *chip, uint16_t opcode) {
    uint8_t ox = OP_X(opcode);
    set_V(chip, ox, (get_V(chip, ox) + OP_LOW_BYTE(opcode)));
}

/**
 * 8XYN - Arithmetic and Logical operations
 * --------------------------------------------------
 * 8XY0: LD   Vx, Vy        -> Vx = Vy
 * 8XY1: OR   Vx, Vy        -> Vx = Vx | Vy
 * 8XY2: AND  Vx, Vy        -> Vx = Vx & Vy
 * 8XY3: XOR  Vx, Vy        -> Vx = Vx ^ Vy
 * 8XY4: ADD  Vx, Vy        -> Vx = Vx + Vy; VF = carry
 * 8XY5: SUB  Vx, Vy        -> Vx = Vx - Vy; VF = NOT borrow (1 if Vx > Vy, else 0)
 * 8XY6: SHR  Vx, Vy        -> Vx = Vy >> 1; VF = least-significant bit before shift
 * 8XY7: SUBN Vx, Vy        -> Vx = Vy - Vx; VF = NOT borrow (1 if Vy > Vx, else 0)
 * 8XYE: SHL  Vx, Vy        -> Vx = Vy << 1; VF = most-significant bit before shift
 */
static void op_8(Chip8 *chip, uint16_t opcode) {
    switch(OP_NIBBLE(opcode)){
        case(0x0): {
            set_V(chip, OP_X(opcode),
                get_V(chip, OP_Y(opcode))
                );
            break;
        }

        case(0x1): {
            chip->V[OP_X(opcode)] |= chip->V[OP_Y(opcode)];
            break;
        }
        case(0x2): {
            chip->V[OP_X(opcode)] &= chip->V[OP_Y(opcode)];
            break;
        }

        case(0x3): {
            chip->V[OP_X(opcode)] ^= chip->V[OP_Y(opcode)];
            break;
        }

        case(0x4): {
            uint8_t ox = OP_X(opcode);
            uint8_t Vy = get_V(chip, OP_Y(opcode));
            uint8_t Vx = get_V(chip, ox);
            uint16_t sum = Vx + Vy;
            set_V(chip, ox, (uint8_t)(sum & 0xFF));
            set_V(chip, 0xF, (sum > 0xFF) ? 1 : 0);
            break;
        }

        case(0x5): {
            uint8_t ox = OP_X(opcode);
            uint8_t Vy = get_V(chip, OP_Y(opcode));
            uint8_t Vx = get_V(chip, ox);
            uint8_t flag = (Vx >= Vy) ? 1 : 0;
            set_V(chip, ox, Vx - Vy);
            set_V(chip, 0xF, flag);
            break;
        }

        case(0x6): {
            uint8_t ox = OP_X(opcode);
            uint8_t Vy = get_V(chip, OP_Y(opcode));
            uint8_t flag = Vy & 0x1;
            set_V(chip, ox, Vy >> 1);
            set_V(chip, 0xF, flag);
            break;
        }

        case(0x7): {
            uint8_t ox = OP_X(opcode);
            uint8_t Vy = get_V(chip, OP_Y(opcode));
            uint8_t Vx = get_V(chip, ox);
            uint8_t flag = (Vy >= Vx) ? 1 : 0;
            set_V(chip, ox, Vy - Vx);
            set_V(chip, 0xF, flag);
            break;
        }

        case(0xE): {
            uint8_t ox = OP_X(opcode);
            uint8_t val = get_V(chip, OP_Y(opcode));
            uint8_t flag = (val & 0x80) >> 7;
            set_V(chip, ox, val << 1);
            set_V(chip, 0xF, flag);
            break;
        }
        
        default:
            op_unknown(chip, opcode);
    }
}

/**
 * 9XY0 - SNE Vx, Vy
 * --------------------------------------------------
 * Skip next instruction if Vx != Vy:
 */
static void op_9(Chip8 *chip, uint16_t opcode) {
    if(OP_NIBBLE(opcode) == 0x0){
        uint8_t Vx = get_V(chip, OP_X(opcode));
        uint8_t Vy = get_V(chip, OP_Y(opcode));
        if (Vx != Vy){
            pc_incr(chip);
        }
    } else {
        op_unknown(chip, opcode);
    }
}

/**
 * ANNN - LD I, addr
 * --------------------------------------------------
 * Set index register I = NNN
 */
static void op_A(Chip8 *chip, uint16_t opcode) {
	chip->I = OP_ADDR(opcode);
}

/**
 * BNNN - JP V0, addr
 * --------------------------------------------------
 * Jump to location NNN + V0
 */
static void op_B(Chip8 *chip, uint16_t opcode) {
    uint16_t addr = OP_ADDR(opcode) + get_V(chip, 0x0);
    pc_set(chip, addr);
}

/**
 * CXNN - RND Vx, byte
 * --------------------------------------------------
 * Set Vx = (random byte between 0 and 255) & NN
 */
static void op_C(Chip8 *chip, uint16_t opcode) {
    uint8_t rand_num = rand() & 0xFF;
	set_V(chip, OP_X(opcode), rand_num & OP_LOW_BYTE(opcode));
}

/**
 * DXYN - DRW Vx, Vy, nibble
 * --------------------------------------------------
 * Draw N-byte sprite starting at memory location I at coordinate (Vx, Vy).
 * Screen pixels are XORed with the sprite bits.
 * VF is set to 1 if any existing screen pixels are erased, else 0.
 */
static void op_D(Chip8 *chip, uint16_t opcode) {
    uint8_t Vx = get_V(chip, OP_X(opcode)) % CHIP8_WIDTH;
    uint8_t Vy = get_V(chip, OP_Y(opcode)) % CHIP8_HEIGHT;
    uint8_t sprite_height = OP_NIBBLE(opcode);
    // Collision flag set to 0
    set_V(chip, 0xF, 0);
    for(size_t i = 0; i < sprite_height; i++){
        // Get each row of the sprite
        uint8_t sprite_row = get_mem_byte(chip, chip->I + i);

        // Shift it to where it is needed horizontally
        uint64_t row = ((uint64_t)sprite_row << (CHIP8_WIDTH - 8)) >> Vx;

        // If calculated row to draw on is out of bounds do nothing
        size_t screen_y = (Vy + i);
        if (screen_y >= CHIP8_HEIGHT) break;

        // If collision with XOR detected, set flag to 1
        if (chip->screen[screen_y] & row) {
            set_V(chip, 0xF, 1);
        }

        // Place into the screen appropiate vertical coordinate
        chip->screen[screen_y] ^= row;
    }
    // Indicate need for rendering
    chip->draw_flag = true;
}

/**
 * EXNN - Keyboard conditionals
 * --------------------------------------------------
 * EX9E: SKP  Vx -> Skip next instruction if key in Vx is pressed
 * EXA1: SKNP Vx -> Skip next instruction if key in Vx is NOT pressed
 */
static void op_E(Chip8 *chip, uint16_t opcode) {
	uint8_t key = get_V(chip, OP_X(opcode));
	if(key >= CHIP8_KEYCOUNT){
	    return;
	}
	uint16_t key_mask = (uint16_t)1 << (CHIP8_KEYCOUNT - 1 - key);
	switch (OP_LOW_BYTE(opcode)) {
	    case(0x9E): {
			if(chip->keypad & key_mask){
			    pc_incr(chip);
			}
			break;
		}
		case(0xA1): {
			if(!(chip->keypad & key_mask)){
			    pc_incr(chip);
			}
		    break;
		}
		default: {
		    op_unknown(chip, opcode);
		}
	}
}

/**
 * FXNN - Timers, Memory, and BCD utilities
 * --------------------------------------------------
 * FX07: LD Vx, DT     -> Vx = delay_timer
 * FX0A: LD Vx, K      -> Wait for key press and store its value in Vx
 * FX15: LD DT, Vx     -> delay_timer = Vx
 * FX18: LD ST, Vx     -> sound_timer = Vx
 * FX1E: ADD I, Vx     -> I = I + Vx
 * FX29: LD F, Vx      -> I = address of 5-byte font sprite for hexadecimal digit Vx
 * FX33: LD B, Vx      -> Store BCD representation of Vx at memory[I..I+2]
 * FX55: LD [I], Vx    -> Store registers V0 through Vx in memory starting at address I
 * FX65: LD Vx, [I]    -> Read registers V0 through Vx from memory starting at address I
 */
static void op_F(Chip8 *chip, uint16_t opcode) {
	switch(OP_LOW_BYTE(opcode)){
	    case(0x07): {
			set_V(chip, OP_X(opcode), chip->delay_timer);
			break;
		}

	    case(0x0A): {
			// If no key pressed, wait
			if(chip->keypad == 0){
			    chip->pc -= 2;
				return;
			}

			// If some key pressed, check wether it already was
			for (size_t k = 0; k < CHIP8_KEYCOUNT; k++) {
                size_t pos = CHIP8_KEYCOUNT - 1 - k;
                bool pressed_now = (chip->keypad >> pos) & 1;
                bool pressed_before = (chip->prev_keypad >> pos) & 1;
                // If it wasnt, store it
                if (pressed_now && !pressed_before) {
                    set_V(chip, OP_X(opcode), (uint8_t)k);
                    return;
                }
			}
			// If every key was in the same state as before, wait
			chip->pc -= 2;
			break;
		}

	    case(0x15): {
			chip->delay_timer = get_V(chip, OP_X(opcode));
			break;
		}

	    case(0x18): {
			chip->sound_timer = get_V(chip, OP_X(opcode));
			break;
		}

	    case(0x1E): {
			chip->I += get_V(chip, OP_X(opcode));
			break;
		}

	    case(0x29): {
			chip->I = (5 * (get_V(chip, OP_X(opcode)) & 0x0F)) + FONTSET_START;
			break;
		}

	    case(0x33): {
			uint8_t Vx = get_V(chip, OP_X(opcode));
			set_mem_byte(chip, chip->I, Vx/100);
			set_mem_byte(chip, chip->I + 1, (Vx%100)/10);
			set_mem_byte(chip, chip->I + 2, Vx%10);
			break;
		}

	    case(0x55): {
			for(size_t i = 0; i <= OP_X(opcode); i++){
			    set_mem_byte(chip, chip->I + i, get_V(chip, i));
			}
			break;
		}

	    case(0x65): {
			for(size_t i = 0; i <= OP_X(opcode); i++){
			    set_V(chip, i, get_mem_byte(chip, chip->I + i));
			}
			break;
		}

		default: {
		    op_unknown(chip, opcode);
		}
	}
}
