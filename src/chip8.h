/*
 * Headers for functionality supported by the emulator.
 */
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define CHIP8_WIDTH             64
#define CHIP8_HEIGHT            32
#define CHIP8_PIXELCOUNT        (32*64)
#define CHIP8_TIMER_HZ          60
#define CHIP8_CPU_HZ            600
#define CHIP8_MAX_PROGRAM_SIZE  (4096 - 0x200)

typedef struct Chip8 Chip8;

// Fully initializes a chip.
Chip8* chip8_init();
// Checks wether the chip has crashed or not
bool chip8_crashed(Chip8 *chip);
// Check the state of a screen pixel
bool chip8_get_pixelstate(Chip8 *chip, size_t n);
// Get flag that indicates if rendering is needed
bool chip8_get_drawflag(Chip8 *chip);
// Get flag that indicates if sound playing is needed
bool chip8_get_soundflag(Chip8 *chip);
// Notify the chip after rendering
void chip8_end_draw(Chip8 *chip);
// Notify the chip wether a certain key is pressed or not
void chip8_notify_keypad_state(Chip8 *chip, size_t i, bool ispressed);
// Decrement timer call, to be executed 60 times each second.
void chip8_update_timers(Chip8 *chip);
// Loads a program into the chip memory of a specific length
void chip8_load(Chip8 *chip, const uint8_t *program, size_t size);
// Advances a cycle of execution
void chip8_cycle(Chip8 *chip);
// Dumping the status of the chip after error for debugging purposes
void dump_chip_status(FILE *file, Chip8 *chip);
// Destroys a chip.
void chip8_destroy(Chip8 *chip);

#endif //CHIP8-H
