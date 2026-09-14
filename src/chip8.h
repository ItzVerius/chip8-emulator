/*
 * Headers for functionality supported by the emulator.
 */
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define CHIP8_TIMER_HZ 60
#define CHIP8_CPU_HZ 600
#define RESOLUTION 32*64
#define SCREENSIZE RESOLUTION/64
#define STACKSIZE 16
#define KILOBYTES(x) (x)*1024
#define MEMORYSIZE KILOBYTES(4)
#define REGCOUNT 16

typedef struct Chip8 Chip8;

extern const uint8_t chip8_fontset[80];

// Fully initializes a chip.
Chip8* chip8_init();
// Loads a program into the chip memory of a specific length
void chip8_load(Chip8 *chip, const uint8_t *program, size_t size);
// Advances a cycle of execution
void chip8_cycle(Chip8 *chip);
// Destroys a chip.
void chip8_destroy(Chip8 *chip);
// Dumping the status of the chip for debugging purposes
void dump_chip_status(FILE *file, Chip8 *chip);
#endif
