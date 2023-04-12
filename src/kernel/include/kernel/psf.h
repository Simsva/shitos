#ifndef KERNEL_PSF_H_
#define KERNEL_PSF_H_

#include <stdint.h>
#include <stddef.h>

#define PSF1_MAGIC      0x0434
#define PSF1_MODE512    0x01    /* 512 glyphs, otherwise 256 */
#define PSF1_MODEHASTAB 0x02    /* has unicode table */
#define PSF1_MODESEQ    0x04    /* equivalent to PSF1_MODEHASTAB */

#define PSF2_MAGIC             0x864ab572
#define PSF2_HAS_UNICODE_TABLE 0x00000001   /* has unicode table */

typedef struct psf1_hdr {
    uint16_t magic;        /* PSF1_MAGIC */
    uint8_t font_mode;     /* PSF1_MODE* */
    uint8_t char_sz;       /* character size */
} psf1_hdr_t;

typedef struct psf2_hdr {
    uint32_t magic;        /* PSF2_MAGIC */
    uint32_t version;      /* zero */
    uint32_t hdr_sz;       /* size of header, offset of bitmaps */
    uint32_t flags;        /* PSF2 flags */
    uint32_t num_glyphs;   /* number of glyphs */
    uint32_t glyph_sz;     /* size of one glyph in bytes */
    uint32_t height;       /* glyph height in pixels */
    uint32_t width;        /* glyph width in pixels */
} psf2_hdr_t;

typedef struct psf_file {
    size_t sz;             /* file size */
    uint32_t *utf16_map;   /* UTF16 to glyph map */
    uint8_t version;       /* PSF version, 1 or 2 */
    uint8_t file[];        /* file contents */
} psf_file_t;

psf_file_t *psf_open(const char *path);
void psf_free(psf_file_t *psf);

void psf_generate_utf16_map(psf_file_t *psf);
uint32_t psf_get_glyph_unicode(psf_file_t *psf, uint16_t c);
uint32_t psf_get_glyph(psf_file_t *psf, uint32_t c);
void *psf_get_bitmap(psf_file_t *psf, uint16_t glyph);

#endif // KERNEL_PSF_H_
