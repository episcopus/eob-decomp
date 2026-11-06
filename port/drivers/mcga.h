/* mcga.h: what the files of MCGA.OVL in C share (playbook 80, M4; src/drivers/MCGA.ASM, whitepaper 2.5).

   MCGA.OVL is the graphics driver for the VGA choice: BIOS mode 13h, one byte a pixel, a screen line
   320 bytes, a page 64000 bytes. Its C is written from the matched TASM source, which stays the
   reference, one routine for each PROC, named as the source names it; each routine's comment gives
   its offset in the driver (`MCGA.OVL 04F8`).

   The driver lives in the arena where the game loaded it (DOS 4B03h, segment mcga_cs) with its
   variables and tables at their offsets in that segment (its `cs:` variables), so the game, which
   reads some of them through its slots (the row offsets, the remap table), and the per-function test
   see them where the original keeps them. Its exports are entered with their argument bytes
   (driver.h): `arg_word(arg, 6)` is the word the source reads at [bp+6].

   Every routine of the source saves the registers it uses but AX where it returns a value, so only
   its value (DX:AX as the table in mcga.c says) and memory are its effects. */
#ifndef PORT_MCGA_H
#define PORT_MCGA_H

#include "tcport.h"
#include "machine.h"
#include "driver.h"
#include "../asm/x86.h"

#define SCREEN_WIDTH 320u
#define SCREEN_HEIGHT 200u
#define SCREEN_COLS 40u            /* 8-pixel columns in a line */
#define PAGE_WORDS 32000u          /* words in a 64000-byte page */
#define MASK_PLANE 8000u           /* bytes in one bit plane of page 4 */
#define FONT_HEIGHT 0x102u         /* the font's byte offsets: its height in lines */
#define FONT_WIDTH 0x103u          /* and its width in pixels */

/* the segment the driver was loaded at (mcga_register) */
extern u16 mcga_cs;

/* the arguments: the word and the far pointer at [bp+off] of a far routine ([bp+6] is the first) */
static inline u16 arg_word(const u8 *arg, unsigned off) { return tc_vr16(arg + off - 6u); }
static inline farptr arg_far(const u8 *arg, unsigned off) { return tc_vr32(arg + off - 6u); }

/* the driver's own variables (cs:) */
#define CS(off) TC_MKFP(mcga_cs, (off))
static inline u8 cs8(u16 off) { return tc_ld_u8(CS(off)); }
static inline u16 cs16(u16 off) { return tc_ld_u16(CS(off)); }
static inline farptr cs32(u16 off) { return tc_ld_u32(CS(off)); }
static inline void set_cs8(u16 off, u8 v) { tc_st_u8(CS(off), v); }
static inline void set_cs16(u16 off, u16 v) { tc_st_u16(CS(off), v); }
static inline void set_cs32(u16 off, farptr v) { tc_st_u32(CS(off), v); }

/* cs:0003: the slot table, seven imports from the game, then the forty exports (cs:001F) */
#define G_CUR_FONT_PTR 0x0003u      /* slot 0: ds:FE4E, the far pointer to the current font */
#define G_PAGE_SEGS_PTR 0x0007u     /* slot 1: ds:FC0C, the eight page segments */
#define G_DRAW_PAGE_PTR 0x000Bu     /* slot 2: ds:8115, the page the driver draws to */
#define G_MEM_ALLOC_PTR 0x000Fu     /* slot 3: 19BC:0007 MemAlloc */
#define G_MEM_FREE_PTR 0x0013u      /* slot 4: 19BC:0137 MemFree */
#define G_GET_FAR_POINTER_PTR 0x0017u
#define G_SET_FAR_POINTER_PTR 0x001Bu
#define G_EXPORTS 0x001Fu
#define G_SLOT_COUNT 0x00BFu

/* cs:00C1: the region and the shape draws' state */
#define G_SAVED_LEFT 0x00C1u        /* SaveDrawState keeps the region here */
#define G_SAVED_RIGHT 0x00C3u
#define G_SAVED_TOP 0x00C5u
#define G_SAVED_BOTTOM 0x00C7u
#define G_FILL_COLOUR 0x00C9u       /* byte: FillRect's colour */
#define G_REGION_LEFT 0x00CAu       /* the region SetRegion sets: columns */
#define G_REGION_RIGHT 0x00CCu
#define G_REGION_TOP 0x00CEu        /* and lines */
#define G_REGION_BOTTOM 0x00D0u
#define G_SHAPE_SEG 0x00D2u
#define G_SHAPE_X 0x00D4u
#define G_SHAPE_Y 0x00D6u
#define G_SHAPE_ROWS 0x00D8u
#define G_SHAPE_WIDTH 0x00DAu
#define G_SHAPE_FULL_WIDTH 0x00DCu
#define G_REGION_COLS 0x00DEu
#define G_SKIP_LEFT 0x00E0u
#define G_SKIP_RIGHT 0x00E2u
#define G_VISIBLE_WIDTH 0x00E4u
#define G_ROW_BYTES 0x00E6u
#define G_SRC_ROW_SKIP 0x00E8u
#define G_SRC_ROW_START 0x00EAu
#define G_DST_ROW_SKIP 0x00ECu
#define G_ROWS_LEFT 0x00EEu
#define G_DST_SEG 0x00F0u
#define G_SHAPE_UNUSED 0x00F2u
#define G_ODD_X 0x00F4u
#define G_NIBBLE_SWAP 0x00F6u       /* 256 bytes: each byte with its nibbles swapped */

/* cs:01F6: the masked draws' pages, the sprite cutters' counts, SetLineClip's rectangle */
#define G_MASK_PLANE_SEG 0x01F6u
#define G_BACK_PAGE_SEG 0x01F8u
#define G_MASK_BIT 0x01FAu          /* byte */
#define G_MASK_LEVEL 0x01FBu
#define G_CELL_BYTES 0x01FDu
#define G_NIBBLE_HIGH 0x0203u       /* byte */
#define G_LINE_CLIP_LEFT 0x0204u
#define G_LINE_CLIP_TOP 0x0206u
#define G_LINE_CLIP_RIGHT 0x0208u
#define G_LINE_CLIP_BOTTOM 0x020Au
#define G_COLOUR_MAP 0x020Cu        /* 256 bytes */
#define G_REMAP_TABLE 0x030Cu       /* 16 bytes */
#define G_CELL_SEG 0x031Cu
#define G_CELL_OFF 0x031Eu
#define G_CELL_SRC_ROW 0x0322u
#define G_CELL_ROW_START 0x0324u
#define G_CELL_ROWS 0x0326u         /* byte */
#define G_COLOURS_USED 0x0327u      /* byte */
#define G_ROW_OFFSETS 0x0328u       /* 201 words: y * 320 */

/* cs:04C6: the tile draws' state */
#define G_TILE_COLOURS_PTR 0x04C6u
#define G_TILE_MAP_PTR 0x04CAu
#define G_TILE_DST_SKIP 0x04CEu
#define G_TILE_ROWS 0x04D0u
#define G_TILE_ROW_BYTES 0x04D2u
#define G_TILE_HEIGHT_CELLS 0x04D4u
#define G_TILE_PARAS 0x04D6u
#define G_TILE_PIXELS_PTR 0x04D8u
#define G_TILE_WIDTH_CELLS 0x04DCu
#define G_TILE_MODE 0x04DEu
#define G_TILE_FORMAT_ARG3 0x04E0u
#define G_TILE_FORMAT_ARG4 0x04E2u
#define G_TILE_REQ_WIDTH 0x04E4u
#define G_TILE_REQ_HEIGHT 0x04E6u
#define G_TILE_COLOUR_ROW 0x04E8u   /* 16 bytes */

/* the offset of screen line y, from the table at cs:0328 (y a word index, as `shl bx,1` makes it) */
static inline u16 row_offset(u16 y) { return cs16((u16)(G_ROW_OFFSETS + 2u * y)); }

/* the page helpers (mcga.c): segments from the game's gPageSegs */
u16 mcga_NextPageSeg(void);              /* 0AFA: the page after the draw page */
u16 mcga_DrawPageSeg(void);              /* 0B1D: the draw page */
u16 mcga_PageSeg(u16 page);              /* 0B3F: page & 1Fh */

/* the game's routines the driver calls through its import slots (mcga.c), with DS as the driver has
   it at the call */
farptr mcga_mem_alloc(u16 ds, u16 size_low, u16 size_high, u16 flags, const char *site);

/* The routines, by file: each file has a table of the routines it has written (offset, name, C,
   argument bytes, bytes of DX:AX returned, other registers answered; driver.h), and a routine is
   registered once its row is there. The exports are entered with their argument bytes. A file's
   routines are static, under the source's names; a global of the driver is named mcga_ and the
   source's name, since the game has routines of the same names (its own CopyPage, 19EC:002A). */
extern const tc_driver_routine mcga_core_routines[];       /* mcga.c: entry, link, the small exports */
extern const unsigned mcga_core_count;
extern const tc_driver_routine mcga_rect_routines[];       /* mcga_rect.c: rectangles, cells, tiles, lines */
extern const unsigned mcga_rect_count;
extern const tc_driver_routine mcga_cell_routines[];       /* mcga_cell.c: the sprite cutters, the text */
extern const unsigned mcga_cell_count;
extern const tc_driver_routine mcga_shape_routines[];      /* mcga_shape.c: DrawShape and its mirror */
extern const unsigned mcga_shape_count;
extern const tc_driver_routine mcga_mask_routines[];       /* mcga_mask.c: the masked and remapped draws */
extern const unsigned mcga_mask_count;

#endif
