/*
 * Headers for functionality supported by the emulator.
 */
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stddef.h>

#define CHIP8_TIMER_HZ 60
#define CHIP8_CPU_HZ 600

// System representation
typedef struct {
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
} Chip8;

extern const uint8_t chip8_fontset[80];

// Fully initializes a chip given a pointer to assign it to.
void chip8_init(Chip8 *chip);
// Loads a program into the chip memory
void chip8_load(Chip8 *chip, const uint8_t *program, size_t size);
// Advances a cycle of execution
void chip8_cycle(Chip8 *chip);

#endif
