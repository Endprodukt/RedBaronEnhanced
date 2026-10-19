#include "capture/capture.h"

#include "core/rb_symbols.h"

#include <string.h>

void rbcap_watch_set(uint8_t *watch)
{
    memset(watch, 0, 0x3000);
    memset(watch, 1, 0x180);                    /* RAM (0180-01FF is the stack) */
    memset(watch + 0x200, 1, 0x200);
    watch[RB_TESTSW] = watch[RB_OPT0] = watch[RB_OPT1] = 1;
    memset(watch + RB_EAIN, 1, 0x40);
}

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int rbcap_open(rbcap_reader *r, const char *path)
{
    memset(r, 0, sizeof *r);
    r->f = fopen(path, "rb");
    if (!r->f) return -1;
    char magic[8];
    if (fread(magic, 1, 8, r->f) != 8 || memcmp(magic, "RBCAP1\0\0", 8) != 0) {
        fclose(r->f);
        r->f = NULL;
        return -1;
    }
    return 0;
}

void rbcap_close(rbcap_reader *r)
{
    if (r->f) fclose(r->f);
    r->f = NULL;
}

int rbcap_next_chunk(rbcap_reader *r, char tag[4], uint8_t *buf, uint32_t cap, uint32_t *len)
{
    uint8_t head[8];
    size_t n = fread(head, 1, 8, r->f);
    if (n == 0) return 0;
    if (n != 8) return -1;
    memcpy(tag, head, 4);
    *len = u32(head + 4);
    if (memcmp(tag, "ROMS", 4) == 0 && *len == sizeof r->rom) {
        if (fread(r->rom, 1, sizeof r->rom, r->f) != sizeof r->rom) return -1;
        r->have_rom = 1;
        return 1;
    }
    uint32_t take = *len < cap ? *len : cap;
    if (take && fread(buf, 1, take, r->f) != take) return -1;
    if (*len > take && fseek(r->f, (long)(*len - take), SEEK_CUR) != 0) return -1;
    return 1;
}

void rbcap_parse_frame(const uint8_t *p, rbcap_frame *t)
{
    t->frame = u32(p);
    t->nmis = u32(p + 4);
    p += 8;
    memcpy(t->ram, p, RBCAP_RAM_SIZE);
    p += RBCAP_RAM_SIZE;
    memcpy(t->vgram, p, RBCAP_VGRAM_SIZE);
    p += RBCAP_VGRAM_SIZE;
    for (int i = 0; i < 16; i++) t->mb_reg[i] = (int16_t)(uint16_t)(p[2 * i] | (p[2 * i + 1] << 8));
    t->mb_result = (int16_t)(uint16_t)(p[32] | (p[33] << 8));
}
