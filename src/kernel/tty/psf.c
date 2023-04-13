/**
 * @brief PC Screen Font parser
 */
/* TODO: UTF-8 support */
#include <kernel/psf.h>
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <string.h>
#include <assert.h>

/**
 * Returns a psf_file struct containing information about the font.
 * Freed by the caller. Returns NULL on error.
 */
psf_file_t *psf_open(const char *path) {
    fs_node_t *file = kopen(path, 0);
    if(!file) return NULL;

    psf_file_t *psf = kmalloc(sizeof(psf_file_t) + file->sz);
    /* memset(psf, 0, sizeof(psf_file_t) + file->sz); */
    fs_read(file, 0, file->sz, psf->file);
    fs_close(file);

    if(((psf1_hdr_t *)psf->file)->magic == PSF1_MAGIC)
        psf->version = 1;
    else if(((psf2_hdr_t *)psf->file)->magic == PSF2_MAGIC)
        psf->version = 2;
    else {
        kfree(psf);
        return NULL;
    }

    psf->sz = file->sz;
    psf->utf16_map = NULL;

    return psf;
}

/**
 * Free a psf_file structure.
 */
void psf_free(psf_file_t *psf) {
    if(!psf) return;
    if(psf->utf16_map) kfree(psf->utf16_map);
    kfree(psf);
}

/**
 * Generates psf->utf16_map
 */
void psf_generate_utf16_map(psf_file_t *psf) {
    switch(psf->version) {
    case 1:
        {
            psf1_hdr_t *hdr = (void *)psf->file;
            psf->utf16_map = kmalloc(sizeof(uint32_t) * (1<<16));

            size_t glyph_sz = (hdr->font_mode & PSF1_MODE512 ? 512 : 256)
                            * hdr->char_sz;
            uint16_t *s = (void *)hdr + sizeof *hdr + glyph_sz;

            uint16_t i = 0;
            while((uintptr_t)s - (uintptr_t)hdr < psf->sz)
                psf->utf16_map[i++] = *s++;
        }
        break;

    case 2:
        {
            psf2_hdr_t *hdr = (void *)psf->file;
            psf->utf16_map = kmalloc(sizeof(uint32_t) * (1<<16));
            memset(psf->utf16_map, 0, sizeof(uint32_t) * (1<<16));

            size_t glyph_sz = hdr->glyph_sz * hdr->num_glyphs;
            uint8_t *s = (void *)hdr + hdr->hdr_sz + glyph_sz;

            uint16_t i = 0;
            while((uintptr_t)s - (uintptr_t)hdr < psf->sz) {
                uint16_t uc = s[0];
                if(uc == 0xff) {
                    i++, s++;
                    continue;
                } else if(uc & 0x80) {
                    /* UTF-8 to UTF-16 */
                    if((uc & 0x20) == 0) {
                        uc = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
                        s++;
                    } else if((uc & 0x10) == 0) {
                        uc = ((((s[0] & 0xf) << 6) + (s[1] & 0x3f)) << 6)
                              + (s[2] & 0x3f);
                        s += 2;
                    } else if((uc & 0x8) == 0) {
                        uc = ((((((s[0] & 0x7) << 6) + (s[1] & 0x3f)) << 6)
                                + (s[2] & 0x3f)) << 6) + (s[3] & 0x3f);
                        s += 3;
                    } else
                        uc = 0;
                }

                psf->utf16_map[uc] = i;
                s++;
            }
        }
        break;
    }
}

/**
 * Get a glyph index from a UTF-16 char. Requires psf->utf16_map.
 */
uint32_t psf_get_glyph_unicode(psf_file_t *psf, uint16_t c) {
    if(!psf->utf16_map) return 0;

    return psf->utf16_map[c];
}

/**
 * Returns the glyph index from a unicode character.
 */
uint32_t psf_get_glyph(psf_file_t *psf, uint32_t c) {
    switch(psf->version) {
    case 1:
        {
            psf1_hdr_t *hdr = (void *)psf->file;
            if(hdr->font_mode & (PSF1_MODEHASTAB | PSF1_MODESEQ))
                return psf_get_glyph_unicode(psf, c);
            else
                return c;
        }

    case 2:
        {
            psf2_hdr_t *hdr = (void *)psf->file;
            if(hdr->flags & PSF2_HAS_UNICODE_TABLE)
                return psf_get_glyph_unicode(psf, c);
            else
                return c;
        }
    }

    return 0;
}

/**
 * Get a pointer to the beginning of the glyph bitmap of the given glyph.
 */
void *psf_get_bitmap(psf_file_t *psf, uint16_t glyph) {
    switch(psf->version) {
    case 1:
        {
            psf1_hdr_t *hdr = (void *)psf->file;
            void *out = (void *)hdr + sizeof *hdr + glyph*hdr->char_sz;
            size_t glyph_sz = (hdr->font_mode & PSF1_MODE512 ? 512 : 256)
                            * hdr->char_sz;
            return (uintptr_t)out < (uintptr_t)hdr + sizeof *hdr + glyph_sz
                ? out
                : NULL;
        }

    case 2:
        {
            psf2_hdr_t *hdr = (void *)psf->file;
            return glyph < hdr->num_glyphs
                ? (void *)hdr + hdr->hdr_sz + glyph*hdr->glyph_sz
                : NULL;
        }
    }

    return NULL;
}
