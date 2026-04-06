/* stdint.h does not exist on FreeBSD 4.x; its types are defined in sys/types.h instead */
#if defined(__FreeBSD__) && (__FreeBSD__ < 5)
#include <sys/types.h>

/* Likewise, on Sun, standard integer types are in sys/types.h */
#elif defined(__sun__)
#include <sys/types.h>

/* EFI does not have stdint.h, or anything else equivalent */
#elif defined(EFI32) || defined(EFI64) || defined(EFIX64)
#include "Tiano.h"
#if !defined(_STDINT_H_)
typedef UINT8 uint8_t;
typedef INT8 int8_t;
typedef UINT16 uint16_t;
typedef INT16 int16_t;
typedef UINT32 uint32_t;
typedef INT32 int32_t;
#endif
/* Windows has its own differences */
#elif defined(_WIN32)
#include <windows.h>
#define _UNUSED
#ifndef _MSL_STDINT_H
typedef UINT8 uint8_t;
typedef INT8 int8_t;
typedef UINT16 uint16_t;
typedef INT16 int16_t;
typedef UINT32 uint32_t;
typedef INT32 int32_t;
#endif

/* All other Posix platforms use stdint.h */
#else
#include <stdint.h>
#endif




// Base 64 encoding according to <https://tools.ietf.org/html/rfc4648#section-4>.
#define kBase64EncodingTable "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

static void base64Encode(char *buffer, size_t buflen, void *rdata, size_t rdlen)
{
    const uint8_t *src = (const uint8_t *)rdata;
    const uint8_t *const end = &src[rdlen];
    char *dst = buffer;
    const char *lim;

    if (buflen == 0) return;
    lim = &buffer[buflen - 1];
    while ((src < end) && (dst < lim))
    {
        uint32_t i;
        const size_t rem = (size_t)(end - src);

        // Form a 24-bit input group. If less than 24 bits remain, pad with zero bits.
        if (     rem >= 3) i = (src[0] << 16) | (src[1] << 8) | src[2]; // 24 bits are equal to 4 6-bit groups.
        else if (rem == 2) i = (src[0] << 16) | (src[1] << 8);          // 16 bits are treated as 3 6-bit groups + 1 pad
        else               i =  src[0] << 16;                           //  8 bits are treated as 2 6-bit groups + 2 pads

        // Encode each 6-bit group.
                       *dst++ =              kBase64EncodingTable[(i >> 18) & 0x3F];
        if (dst < lim) *dst++ =              kBase64EncodingTable[(i >> 12) & 0x3F];
        if (dst < lim) *dst++ = (rem >= 2) ? kBase64EncodingTable[(i >>  6) & 0x3F] : '=';
        if (dst < lim) *dst++ = (rem >= 3) ? kBase64EncodingTable[ i        & 0x3F] : '=';
        src += (rem > 3) ? 3 : rem;
    }
    *dst = '\0';
}







#if defined(_WIN32) && !defined(EFI32) && !defined(EFI64)
#define DNSSD_API __stdcall
#else
#define DNSSD_API
#endif
