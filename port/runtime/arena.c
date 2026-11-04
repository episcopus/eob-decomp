/* arena.c: the arena, the program loader and the code address table (tcport.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"

u8 tc_mem[TC_MEM_SIZE] = {0};
u16 tc_load_seg;
u16 tc_ds;

#define CODE_MAX 4096u

static struct {
    farptr addr;
    tc_fn fn;
    const char *name;
} code[CODE_MAX];
static unsigned code_count;

void tc_code_register(farptr addr, tc_fn fn, const char *name)
{
    unsigned i;
    for (i = 0; i < code_count; i++) {
        if (code[i].addr == addr) {
            code[i].fn = fn;
            code[i].name = name;
            return;
        }
    }
    if (code_count == CODE_MAX) {
        fprintf(stderr, "tcport: code table full at %s\n", name);
        exit(70);
    }
    code[code_count].addr = addr;
    code[code_count].fn = fn;
    code[code_count].name = name;
    code_count++;
}

tc_fn tc_code_lookup(farptr addr, const char *site)
{
    unsigned i;
    for (i = 0; i < code_count; i++) {
        if (code[i].addr == addr) {
            return code[i].fn;
        }
    }
    fprintf(stderr, "tcport: no C function at %04X:%04X (load-relative %04X:%04X), called from %s\n",
            TC_SEG(addr), TC_OFF(addr), (u16)(TC_SEG(addr) - tc_load_seg), TC_OFF(addr), site);
    exit(70);
}

void tc_unported(const char *name, const char *addr)
{
    fprintf(stderr, "tcport: %s (%s) is not ported yet\n", name, addr);
    exit(71);
}

static u16 word_at(const u8 *b) { return (u16)(b[0] | (b[1] << 8)); }

int tc_load_program(const char *path, u16 load_seg, u16 ds_para)
{
    FILE *f;
    long size;
    u8 *file;
    u32 header, image, pages, last, nrel, relofs, i;

    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "tcport: cannot open %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    file = malloc((size_t)size);
    if (file == NULL || fread(file, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "tcport: cannot read %s\n", path);
        fclose(f);
        free(file);
        return -1;
    }
    fclose(f);
    if (size < 0x1C || file[0] != 'M' || file[1] != 'Z') {
        fprintf(stderr, "tcport: %s is not an MZ program\n", path);
        free(file);
        return -1;
    }
    last = word_at(file + 2);
    pages = word_at(file + 4);
    nrel = word_at(file + 6);
    header = (u32)word_at(file + 8) * 16u;
    relofs = word_at(file + 0x18);
    /* The load module: pages of 512 bytes, the last one `last` bytes long (0: full), less the
       header. The FBOV overlay data after it stays in the file, as DOS leaves it. */
    image = pages * 512u - (last ? 512u - last : 0u) - header;
    if ((u32)load_seg * 16u + image > TC_MEM_SIZE || header + image > (u32)size) {
        fprintf(stderr, "tcport: %s does not fit\n", path);
        free(file);
        return -1;
    }
    memcpy(tc_mem + (u32)load_seg * 16u, file + header, image);
    for (i = 0; i < nrel; i++) {
        const u8 *r = file + relofs + i * 4u;
        farptr at = TC_MKFP((u16)(word_at(r + 2) + load_seg), word_at(r));
        tc_st_u16(at, (u16)(tc_ld_u16(at) + load_seg));
    }
    free(file);
    tc_load_seg = load_seg;
    tc_ds = (u16)(load_seg + ds_para);
    return 0;
}
