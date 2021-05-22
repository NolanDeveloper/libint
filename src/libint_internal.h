#ifndef LIBINT_INTERNAL_H
#define LIBINT_INTERNAL_H

#include <libint.h>
#include "libint_utils.h"

#if 1
typedef unsigned             LibintWord;
typedef unsigned long long   LibintDword;
#else
typedef unsigned char        LibintWord;
typedef unsigned short       LibintDword;
#endif

_Static_assert((LibintWord) -1 > 0, "LibintWord must be unsigned");

_Static_assert((LibintDword) -1 > 0, "LibintDword must be unsigned");

_Static_assert(sizeof(LibintWord) * 2 <= sizeof(LibintDword),
               "LibintDword must be at least twice as big as LibintWord");

struct Libint_ {
    Libis *libis;
    LibintSigned *libint_constants[17];
    LibintUnsigned *libint_unsigned_constants[17];
};

struct LibintUnsigned_ {
    size_t size;
    LibintWord *ptr;
};

struct LibintSigned_ {
    bool is_negative;
    LibintUnsigned *magnitude;
};

#define LIBINT_UNSIGNED_INVARIANT(x) \
    ((x)->size == 1 || ((x)->size > 1 && (x)->ptr[(x)->size - 1]))

#define LIBINT_SIGNED_INVARIANT(x) \
    (LIBINT_UNSIGNED_INVARIANT((x)->magnitude) && \
     ((x)->magnitude->size > 1 || (x)->magnitude->ptr[0] || !(x)->is_negative))

#define EIS errorlibis_to_errorlibint

LibintError errorlibis_to_errorlibint(LibisError err);

#define E libint_handle_internal_error

LibintError libint_handle_internal_error(LibintError err);

LibintError libint_construct(Libint *libint, LibintSigned **x, bool is_negative, LibintUnsigned *magnitude);

LibintError libint_unsigned_construct(Libint *libint, LibintUnsigned **x, size_t size, LibintWord *ptr);

LibintError libint_to_string_helper(
        Libint *libint, bool is_negative, LibintUnsigned *x, int base, char **out, size_t *out_size);

#endif
