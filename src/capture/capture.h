/* capture.h - game frames recorded from MAME (tools/mame/rbcapture.lua).
 *
 * File: "RBCAP1\0\0", then chunks: 4-byte tag, uint32 little-endian length,
 * payload. Tags:
 *   ROMS  0x5000 bytes, 3000-7FFF of the recorded machine (once, first)
 *   FRAM  uint32 frame number, uint32 interrupts so far, RAM 0000-03FF,
 *         vector RAM 2000-2FFF, the math box (16 registers and the result,
 *         int16 each): taken when the main line's wait for the frame time
 *         ends, inside INTWAIT's LSR CALFLG that shifts a 1 out (CALFLG
 *         still holds it)
 *   RNDM  the random numbers (181A) the main line read during the frame
 *         before, in order; before each FRAM
 *   IRDS  what the main line read during the frame before of the memory
 *         the interrupt writes and of the hardware (switches, EAROM): 3
 *         bytes per read, address (little-endian) and value, in order. Of
 *         INTWAIT's polls of INTFLG and of the vector generator's halt
 *         only those that found something.
 *   NWRT  the addresses (uint16 each) the interrupt wrote during the frame
 *         before: there the main line's result cannot be compared
 *   TIME  one double, seconds of emulated time at the FRAM that follows
 *   POKE  uint16 address, uint8 value: the recording's script wrote this
 *         into the machine just before the FRAM that follows
 * Unknown tags are skipped, so the format can grow.
 */
#ifndef RB_CAPTURE_H
#define RB_CAPTURE_H

#include <stdint.h>
#include <stdio.h>

#define RBCAP_RAM_SIZE   0x0400
#define RBCAP_VGRAM_SIZE 0x1000
#define RBCAP_FRAM_SIZE  (8 + RBCAP_RAM_SIZE + RBCAP_VGRAM_SIZE + 34)

typedef struct rbcap_frame {
    uint32_t frame;
    uint32_t nmis;
    uint8_t  ram[RBCAP_RAM_SIZE];
    uint8_t  vgram[RBCAP_VGRAM_SIZE];
    int16_t  mb_reg[16];
    int16_t  mb_result;
} rbcap_frame;

typedef struct rbcap_reader {
    FILE   *f;
    uint8_t rom[0x5000];
    int     have_rom;
} rbcap_reader;

int  rbcap_open(rbcap_reader *r, const char *path);   /* 0 or -1 */
void rbcap_close(rbcap_reader *r);

/* The next chunk: its tag and length in *len, its first min(len, cap) bytes
 * in buf. 1, 0 at the end, -1 on a damaged file. */
int  rbcap_next_chunk(rbcap_reader *r, char tag[4], uint8_t *buf, uint32_t cap, uint32_t *len);

/* A FRAM payload (RBCAP_FRAM_SIZE bytes) as a frame. */
void rbcap_parse_frame(const uint8_t *payload, rbcap_frame *t);

/* The addresses whose reads by the main line a recording may hold (IRDS):
 * set to 1 in watch (0x3000 entries: RAM and hardware). */
void rbcap_watch_set(uint8_t *watch);

#endif
