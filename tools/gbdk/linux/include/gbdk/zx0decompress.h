#ifndef __ZX0DECOMPRESS_H_INCLUDE
#define __ZX0DECOMPRESS_H_INCLUDE

#if defined(__TARGET_gb) || defined(__TARGET_ap) || defined(__TARGET_duck) || defined(__TARGET_sms) || defined(__TARGET_gg) || defined(__TARGET_msx)

/** Decompress zx0 compressed data from sour into dest

    @param sour   Pointer to source zx0 compressed data
    @param dest   Pointer to destination buffer/address

    Will decompress __all__ of it's data to destination without
    stopping until the end of compressed data is reached. It is
    not possible to set a limit, so ensure the destination buffer
    has sufficient space to avoid an overflow.

    Decompresses data which has been compressed with @ref utility_gbcompress "gbcompress" using the `--alg=zx0` argument.
*/
void zx0_decompress(void * sour, void * dest);

#endif

#endif