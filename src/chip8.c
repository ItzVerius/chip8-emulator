/*
 * Implementation of emulator
 */
#include "chip8.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

// System representation
struct Chip8{
    uint8_t memory[4096];   // RAM available (4 KB)
    uint8_t V[16];          // 8 bit general purpose registers (except VF, used as a flag by some instructions)
    uint16_t I;             // Register for memory addresses (only 12 lower bits used)
    uint16_t pc;            // Program counter
    uint8_t delay_timer;    // Timer that if not 0, decrement at a 60Hz rate
    uint8_t sound_timer;    // Timer that if not 0, decrement at a 60Hz rate
    uint16_t stack[16];     // Stack for return adresses when calling subroutines
    uint8_t sp;             // Stack pointer
    uint8_t screen[64*32];  // 64 * 32 pixel screen display
    uint8_t keypad[16];     // 4*4 keypad
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


Chip8* chip8_init(){
    Chip8* chip = calloc(1, sizeof(Chip8));
    if(!chip) return NULL;
    chip->pc = 0x200;
    memcpy(&chip->memory[0x50], chip8_fontset, sizeof(chip8_fontset));
    return chip;
}

void chip8_destroy(Chip8 *chip) {
    free(chip);
}

void chip8_load(Chip8 *chip, const uint8_t *program, size_t size){
  assert(size <= (4096 - 0x200));
  memcpy(&chip->memory[0x200], program, size);
}

void chip8_cycle(Chip8 *chip){
    /* Fetch instruction from memory */
    uint16_t opcode = chip->memory[chip->pc];
    opcode = opcode << 8 | chip->memory[chip->pc + 1];
    chip->pc += 2;
    /* Execute instruction */
    uint8_t type = opcode >> 12;


}

/**
 * 
 */

/**
 * 0NNN / 00E0 / 00EE - System / Display / Return
 * --------------------------------------------------
 * 0NNN: SYS addr    -> Execute machine language routine at NNN (ignored)[cite: 1].
 * 00E0: CLS         -> Clear the display[cite: 1].
 * 00EE: RET         -> Return from subroutine: PC = stack[--SP][cite: 1].
 */
static void op_0(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 1NNN - JP addr
 * --------------------------------------------------
 * Jump to location NNN: PC = NNN[cite: 1]
 */
static void op_1(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 2NNN - CALL addr
 * --------------------------------------------------
 * Call subroutine at NNN: stack[SP++] = PC; PC = NNN[cite: 1]
 */
static void op_2(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 3XNN - SE Vx, byte
 * --------------------------------------------------
 * Skip next instruction if Vx == NN:
 * if (Vx == NN) PC += 2[cite: 1]
 */
static void op_3(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 4XNN - SNE Vx, byte
 * --------------------------------------------------
 * Skip next instruction if Vx != NN:
 * if (Vx != NN) PC += 2[cite: 1]
 */
static void op_4(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 5XY0 - SE Vx, Vy
 * --------------------------------------------------
 * Skip next instruction if Vx == Vy:
 * if (Vx == Vy) PC += 2[cite: 1]
 */
static void op_5(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 6XNN - LD Vx, byte
 * --------------------------------------------------
 * Set Vx = NN[cite: 1]
 */
static void op_6(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 7XNN - ADD Vx, byte
 * --------------------------------------------------
 * Set Vx = Vx + NN (lowest 8 bits kept)[cite: 1].
 * Note: VF carry flag is NOT modified[cite: 1].
 */
static void op_7(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 8XYN - Arithmetic and Logical operations
 * --------------------------------------------------
 * 8XY0: LD   Vx, Vy        -> Vx = Vy[cite: 1]
 * 8XY1: OR   Vx, Vy        -> Vx = Vx | Vy[cite: 1]
 * 8XY2: AND  Vx, Vy        -> Vx = Vx & Vy[cite: 1]
 * 8XY3: XOR  Vx, Vy        -> Vx = Vx ^ Vy[cite: 1]
 * 8XY4: ADD  Vx, Vy        -> Vx = Vx + Vy; VF = carry[cite: 1]
 * 8XY5: SUB  Vx, Vy        -> Vx = Vx - Vy; VF = NOT borrow (1 if Vx > Vy, else 0)[cite: 1]
 * 8XY6: SHR  Vx {, Vy}     -> Vx = Vx >> 1; VF = least-significant bit before shift[cite: 1]
 * 8XY7: SUBN Vx, Vy        -> Vx = Vy - Vx; VF = NOT borrow (1 if Vy > Vx, else 0)[cite: 1]
 * 8XYE: SHL  Vx {, Vy}     -> Vx = Vx << 1; VF = most-significant bit before shift[cite: 1]
 */
static void op_8(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * 9XY0 - SNE Vx, Vy
 * --------------------------------------------------
 * Skip next instruction if Vx != Vy:
 * if (Vx != Vy) PC += 2[cite: 1]
 */
static void op_9(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * ANNN - LD I, addr
 * --------------------------------------------------
 * Set index register I = NNN[cite: 1]
 */
static void op_A(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * BNNN - JP V0, addr
 * --------------------------------------------------
 * Jump to location NNN + V0: PC = NNN + V[0][cite: 1]
 */
static void op_B(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * CXNN - RND Vx, byte
 * --------------------------------------------------
 * Set Vx = (random byte between 0 and 255) & NN[cite: 1]
 */
static void op_C(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * DXYN - DRW Vx, Vy, nibble
 * --------------------------------------------------
 * Draw N-byte sprite starting at memory location I at coordinate (Vx, Vy)[cite: 1].
 * Screen pixels are XORed with the sprite bits[cite: 1].
 * VF is set to 1 if any existing screen pixels are erased, else 0[cite: 1].
 */
static void op_D(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * EXNN - Keyboard conditionals
 * --------------------------------------------------
 * EX9E: SKP  Vx -> Skip next instruction if key in Vx is pressed[cite: 1]
 * EXA1: SKNP Vx -> Skip next instruction if key in Vx is NOT pressed[cite: 1]
 */
static void op_E(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}

/**
 * FXNN - Timers, Memory, and BCD utilities
 * --------------------------------------------------
 * FX07: LD Vx, DT     -> Vx = delay_timer[cite: 1]
 * FX0A: LD Vx, K      -> Wait for key press and store its value in Vx[cite: 1]
 * FX15: LD DT, Vx     -> delay_timer = Vx[cite: 1]
 * FX18: LD ST, Vx     -> sound_timer = Vx[cite: 1]
 * FX1E: ADD I, Vx     -> I = I + Vx[cite: 1]
 * FX29: LD F, Vx      -> I = address of 5-byte font sprite for hexadecimal digit Vx[cite: 1]
 * FX33: LD B, Vx      -> Store BCD representation of Vx at memory[I..I+2][cite: 1]
 * FX55: LD [I], Vx    -> Store registers V0 through Vx in memory starting at address I[cite: 1]
 * FX65: LD Vx, [I]    -> Read registers V0 through Vx from memory starting at address I[cite: 1]
 */
static void op_F(Chip8 *chip, uint16_t opcode) {
	(void)chip;
	(void)opcode;
	return;
}