#ifndef LIBINT_H
#define LIBINT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Libint_ Libint;
typedef struct LibintUnsigned_ LibintUnsigned;
typedef struct LibintSigned_ LibintSigned;

typedef enum {
    LIBINT_ERROR_OK,
    LIBINT_ERROR_OUT_OF_MEMORY,
    LIBINT_ERROR_BAD_ARGUMENT,
    // Division by zero or unsigned subtraction with negative result
    // or another error of arithmetical nature.
    LIBINT_ERROR_ARITHMETIC,
    // Input/output error.
    LIBINT_ERROR_IO,
} LibintError;

LibintError libint_start(Libint **libint);

LibintError libint_finish(Libint **libint);

LibintError libint_create(Libint *libint, LibintSigned **x, intmax_t value);

LibintError libint_to_intmax(Libint *libint, LibintSigned *x, intmax_t *value);

LibintError libint_from_string(Libint *libint, LibintSigned **x, const char *input, size_t input_size, int base,
                               const char **input_end);

LibintError libint_to_string(Libint *libint, LibintSigned *x, int base, char **out, size_t *out_size);

LibintError libint_copy(Libint *libint, LibintSigned **out, LibintSigned *x);

LibintError libint_destroy(Libint *libint, LibintSigned **x);

LibintError libint_add(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_sub(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_mul(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_div_trunc(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_mod_trunc(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_div_mod_trunc(
        Libint *libint, LibintSigned **quotient, LibintSigned **remainder, LibintSigned *x, LibintSigned *y);

LibintError libint_div_floor(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_mod_floor(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y);

LibintError libint_div_mod_floor(
        Libint *libint, LibintSigned **quotient, LibintSigned **remainder, LibintSigned *x, LibintSigned *y);

LibintError libint_add_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_sub_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_rsub_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_mul_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_div_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_rdiv_replace(Libint *libint, LibintSigned **x, LibintSigned *y);

LibintError libint_is_zero(Libint *libint, LibintSigned *x, bool *is_zero);

LibintError libint_compare(Libint *libint, LibintSigned *x, LibintSigned *y, int *order);

LibintError libint_less(Libint *libint, LibintSigned *x, LibintSigned *y, bool *out);

LibintError libint_less_or_equal(Libint *libint, LibintSigned *x, LibintSigned *y, bool *out);

LibintError libint_unsigned_create(Libint *libint, LibintUnsigned **x, uintmax_t value);

LibintError libint_unsigned_to_uintmax(Libint *libint, LibintUnsigned *x, uintmax_t *value);

LibintError libint_unsigned_from_string(
        Libint *libint, LibintUnsigned **x, const char *input, size_t input_size, int base, const char **input_end);

LibintError libint_unsigned_to_string(Libint *libint, LibintUnsigned *x, int base, char **out, size_t *out_size);

LibintError libint_unsigned_copy(Libint *libint, LibintUnsigned **out, LibintUnsigned *x);

LibintError libint_unsigned_destroy(Libint *libint, LibintUnsigned **x);

LibintError libint_unsigned_add(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_sub(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_most_significant_bit(Libint *libint, LibintUnsigned *x, size_t *msb);

LibintError libint_unsigned_wordshift(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, int offset);

LibintError libint_unsigned_bitshift(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, int offset);

LibintError libint_unsigned_mul(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_div(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_mod(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_div_mod(
        Libint *libint, LibintUnsigned **out, LibintUnsigned **remainder, LibintUnsigned *x, LibintUnsigned *y);

LibintError libint_unsigned_pow(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, uintmax_t power);

LibintError libint_unsigned_add_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_sub_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_rsub_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_bitshift_replace(Libint *libint, LibintUnsigned **x, int offset);

LibintError libint_unsigned_mul_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_div_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_rdiv_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y);

LibintError libint_unsigned_pow_replace(Libint *libint, LibintUnsigned **x, uintmax_t power);

LibintError libint_unsigned_is_zero(Libint *libint, LibintUnsigned *x, bool *is_zero);

LibintError libint_unsigned_compare(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, int *order);

LibintError libint_unsigned_less(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, bool *out);

LibintError libint_unsigned_less_or_equal(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, bool *out);

#endif
