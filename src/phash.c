#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <wand/magick_wand.h>

#include "phash.h"

uint64_t phash_dct(const char *fname)
{
    uint64_t retval = 0;
    extern int errno;
    
    MagickWandGenesis();
    MagickWand *m_wand = NewMagickWand();

    if (MagickReadImage(m_wand, fname) == MagickFalse) {
        DestroyMagickWand(m_wand);
        MagickWandTerminus();
        errno = EINVAL;
        return retval;
    }

    MagickResizeImage(m_wand, 32, 32, PointFilter, 1.0);

    /* Intensity map */
    double intensity[32][32];
    for (int x = 0; x < 32; ++x) {
        for (int y = 0; y < 32; ++y) {
            PixelWand *colour = NewPixelWand();
            double h, s;

            MagickGetImagePixelColor(m_wand, x, y, colour);
            PixelGetHSL(colour, &h, &s, &intensity[x][y]);

            DestroyPixelWand(colour);
        }
    }
    DestroyMagickWand(m_wand);
    MagickWandTerminus();

    /* Discrete cosine transform */
    double seq[64];
    unsigned seq_i = 0;
    for (int u = 0; u < 8; ++u) {
        for (int v = 0; v < 8; ++v) {
            double acc = 0.0;
            for (int x = 0; x < 32; ++x) {
                for (int y = 0; y < 32; ++y) {
                    acc += intensity[x][y]
                         * cos(M_PI / 32.0 * (x + .5) * u)
                         * cos(M_PI / 32.0 * (y + .5) * v);
                }
            }
            seq[seq_i++] = acc;
        }
    }

    double avg = 0.0;
    for (seq_i = 1; seq_i < 63; ++seq_i)
        avg += seq[seq_i];
    avg /= 63;

    for (seq_i = 0; seq_i < 64; ++seq_i) {
        uint64_t x = seq[seq_i] > avg;
        retval |= x << seq_i;
    }

    return retval;
}

unsigned hamming_uint64_t(uint64_t h1, uint64_t h2)
{
    unsigned ret = 0;
    uint64_t d = h1 ^ h2;

    while (d) {
        ++ret;
        d &= d - 1;
    }

    return ret;
}
