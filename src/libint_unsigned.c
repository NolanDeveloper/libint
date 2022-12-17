#include "libint_internal.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

LibintError libint_unsigned_create(Libint *libint, LibintUnsigned **x, uintmax_t value) {
    LibintError err = LIBINT_ERROR_OK;
    LibintWord *ptr = NULL;
    void *new_ptr = NULL;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *x = NULL;
    size_t size = (sizeof(uintmax_t) + sizeof(LibintWord) - 1) / sizeof(LibintWord);
    ptr = malloc(sizeof(LibintWord) * size);
    if (!ptr) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    size_t i = 0;
    do {
        ptr[i] = value;
        value >>= sizeof(LibintWord) * CHAR_BIT;
        ++i;
    } while (i < size && value);
    size = i;
    new_ptr = realloc(ptr, sizeof(LibintWord) * size);
    if (!new_ptr) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    ptr = new_ptr;
    new_ptr = NULL;
    err = E(libint_unsigned_construct(libint, x, size, ptr));
    if (err) goto end;
    ptr = NULL;
end:
    free(ptr);
    free(new_ptr);
    return err;
}

LibintError libint_unsigned_to_uintmax(Libint *libint, LibintUnsigned *x, uintmax_t *value) {
    LibintError err = LIBINT_ERROR_OK;
    size_t msb;
    if (!libint || !x || !value) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = 0;
    err = E(libint_unsigned_most_significant_bit(libint, x, &msb));
    if (err) goto end;
    if (msb > sizeof(uintmax_t) * CHAR_BIT) {
        err = LIBINT_ERROR_ARITHMETIC;
        goto end;
    }
    uintmax_t result = 0;
    for (size_t i = x->size; i--;) {
        result <<= sizeof(LibintWord) * CHAR_BIT;
        result += x->ptr[i];
    }
    *value = result;
end:
    return err;
}

static bool hex_char_to_int(char c, int *digit) {
    assert(digit);
    *digit = 0;
    if ('0' <= c && c <= '9') {
        *digit = c - '0';
    } else if ('a' <= c && c <= 'f') {
        *digit = 10 + c - 'a';
    } else if ('A' <= c && c <= 'F') {
        *digit = 10 + c - 'A';
    } else {
        return false;
    }
    return true;
}

static bool parse_digit(char c, int base, int *digit) {
    assert(digit);
    if (!hex_char_to_int((char) c, digit)) {
        return false;
    }
    if (base <= *digit) {
        return false;
    }
    return true;
}

LibintError libint_unsigned_from_string(
        Libint *libint, LibintUnsigned **x, const char *input, size_t input_size, int base, const char **input_end) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    LibintUnsigned *base_long = NULL;
    const char *current = input;
    size_t bytes_left = input_size;
    if (!libint || !x || !input || base < 2 || 16 < base || !input_end) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *input_end = NULL;
    *x = NULL;
    err = E(libint_unsigned_create(libint, &result, 0));
    if (err) goto end;
    err = E(libint_unsigned_create(libint, &base_long, base));
    if (err) goto end;
    char c = (char) (bytes_left ? *current : '\0');
    while (true) {
        int digit = -1;
        if (!parse_digit(c, base, &digit)) {
            break;
        }
        err = E(libint_unsigned_mul_replace(libint, &result, base_long));
        if (err) goto end;
        err = E(libint_unsigned_add_replace(libint, &result, libint->libint_unsigned_constants[digit]));
        if (err) goto end;
        if (bytes_left) {
            ++current;
            --bytes_left;
            c = *current;
        } else {
            c = '\0';
        }
    }
    *x = result;
    result = NULL;
end:
    if (input_end) *input_end = current;
    E(libint_unsigned_destroy(libint, &base_long));
    E(libint_unsigned_destroy(libint, &result));
    return err;
}

LibintError libint_unsigned_to_string(Libint *libint, LibintUnsigned *x, int base, char **out, size_t *out_size) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !out || !out_size) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_to_string_helper(libint, false, x, base, out, out_size));
    if (err) goto end;
end:
    return err;
}

LibintError
libint_to_string_helper(Libint *libint, bool is_negative, LibintUnsigned *x, int base, char **out, size_t *out_size) {
    // expansion_factor[i] = ceil(8 * log(2) / log(i + 2))
    static size_t expansion_factor[] = { 8, 6, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2 };
    const char *digits = "0123456789ABCDEF";
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *dividend = NULL;
    LibintUnsigned *quotient = NULL;
    LibintUnsigned *remainder = NULL;
    char *result = NULL;
    char *result_shrinked = NULL;
    if (!libint || !x || !out || !out_size || base < 2 || 16 < base) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    *out_size = 0;
    size_t result_size = 0;
    if (is_negative) {
        result_size += 1; // '-'
    }
    result_size += x->size * sizeof(LibintWord) * expansion_factor[base - 2]; // digits
    result_size += 1; // '\0'
    result = malloc(result_size);
    if (!result) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    err = E(libint_unsigned_copy(libint, &dividend, x));
    if (err) goto end;
    char *it = result;
    int is_not_zero = true;
    while (is_not_zero) {
        err = E(libint_unsigned_div_mod(libint, &quotient, &remainder, dividend,
                                        libint->libint_unsigned_constants[base]));
        if (err) goto end;
        LibintWord digit = remainder->ptr[0];
        assert(remainder->size == 1);
        assert((int) digit < base);
        *it++ = digits[digit];
        E(libint_unsigned_destroy(libint, &dividend));
        E(libint_unsigned_destroy(libint, &remainder));
        dividend = quotient;
        err = E(libint_unsigned_compare(libint, libint->libint_unsigned_constants[0], dividend, &is_not_zero));
        if (err) goto end;
    }
    E(libint_unsigned_destroy(libint, &dividend));
    if (is_negative) {
        *it++ = '-';
    }
    size_t result_real_size = it - result;
    for (size_t i = 0; i < result_real_size / 2; ++i) {
        char t = result[i];
        result[i] = result[result_real_size - 1 - i];
        result[result_real_size - 1 - i] = t;
    }
    *it = '\0';
    result_shrinked = realloc(result, result_real_size + 1);
    if (!result_shrinked) {
        goto end;
    }
    *out_size = result_real_size;
    *out = result_shrinked;
    result_shrinked = NULL;
    result = NULL;
end:
    E(libint_unsigned_destroy(libint, &dividend));
    E(libint_unsigned_destroy(libint, &remainder));
    free(result);
    free(result_shrinked);
    free(dividend);
    return err;
}

LibintError libint_unsigned_construct(Libint *libint, LibintUnsigned **x, size_t size, LibintWord *ptr) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *out = NULL;
    if (!libint || !x || !ptr) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    out = malloc(sizeof(LibintUnsigned));
    if (!out) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    out->size = size;
    out->ptr = ptr;
    assert(LIBINT_UNSIGNED_INVARIANT(out));
    *x = out;
    out = NULL;
end:
    free(out);
    return err;
}

LibintError libint_unsigned_copy(Libint *libint, LibintUnsigned **out, LibintUnsigned *x) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !out || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    err = E(libint_unsigned_add(libint, out, libint->libint_unsigned_constants[0], x));
    if (err) goto end;
end:
    return err;
}

LibintError libint_unsigned_destroy(Libint *libint, LibintUnsigned **x) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*x) {
        free((*x)->ptr);
        free(*x);
        *x = NULL;
    }
end:
    return err;
}

LibintError libint_unsigned_add(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintWord *out_ptr = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (y->size > x->size) {
        LibintUnsigned *t = x;
        x = y;
        y = t;
    }
    *out = NULL;
    size_t out_size = x->size;
    out_ptr = malloc(sizeof(LibintWord) * out_size);
    if (!out_ptr) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    LibintWord *x_it = x->ptr;
    LibintWord *x_end = x->ptr + x->size;
    LibintWord *y_it = y->ptr;
    LibintWord *y_end = y->ptr + y->size;
    LibintWord *it = out_ptr;
    LibintWord carry = 0;
    while (x_it != x_end) {
        LibintDword a = *x_it++;
        LibintDword b = y_it < y_end ? *y_it++ : 0;
        LibintDword c = a + b + carry;
        *it = c;
        carry = c >> (sizeof(LibintWord) * CHAR_BIT);
        ++it;
    }
    if (carry) {
        ++out_size;
        LibintWord *out_ptr_new = realloc(out_ptr, sizeof(LibintWord) * out_size);
        if (!out_ptr_new) {
            err = LIBINT_ERROR_OUT_OF_MEMORY;
            goto end;
        }
        out_ptr = out_ptr_new;
        out_ptr[out_size - 1] = carry;
    }
    err = E(libint_unsigned_construct(libint, out, out_size, out_ptr));
    if (err) goto end;
    out_ptr = NULL;
end:
    free(out_ptr);
    return err;
}

LibintError libint_unsigned_sub(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintWord *out_ptr = NULL;
    void *new_ptr = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    int order;
    err = E(libint_unsigned_compare(libint, x, y, &order));
    if (err) goto end;
    if (order < 0) {
        err = LIBINT_ERROR_ARITHMETIC;
        goto end;
    }
    size_t out_size = x->size > y->size ? x->size : y->size;
    out_ptr = malloc(sizeof(LibintWord) * out_size);
    if (!out_ptr) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    LibintWord *out_it = out_ptr;
    LibintWord *x_it = x->ptr;
    LibintWord *y_it = y->ptr;
    LibintWord *x_end = x->ptr + x->size;
    LibintWord *y_end = y->ptr + y->size;
    bool borrow = false;
    while (y_it != y_end) {
        LibintWord a = *x_it;
        LibintWord b = *y_it;
        *out_it = a - (b + borrow);
        borrow = a < b + borrow;
        ++x_it;
        ++y_it;
        ++out_it;
    }
    while (x_it != x_end) {
        LibintWord a = *x_it;
        *out_it = a - borrow;
        borrow = a < borrow;
        ++x_it;
        ++out_it;
    }
    assert(out_size);
    LibintWord *most_significant_word = out_ptr + out_size - 1;
    while (most_significant_word != out_ptr && !*most_significant_word) {
        --most_significant_word;
    }
    out_size = most_significant_word - out_ptr + 1;
    new_ptr = realloc(out_ptr, sizeof(LibintWord) * out_size);
    if (!new_ptr) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    out_ptr = new_ptr;
    new_ptr = NULL;
    err = E(libint_unsigned_construct(libint, out, out_size, out_ptr));
    if (err) goto end;
    out_ptr = NULL;
end:
    free(out_ptr);
    free(new_ptr);
    return LIBINT_ERROR_OK;
}

LibintError libint_unsigned_most_significant_bit(Libint *libint, LibintUnsigned *x, size_t *msb) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !msb) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *msb = 0;
    bool is_zero;
    err = E(libint_unsigned_is_zero(libint, x, &is_zero));
    if (err) goto end;
    if (is_zero) {
        err = LIBINT_ERROR_ARITHMETIC;
        goto end;
    }
    LibintWord most_significand_word = x->ptr[x->size - 1];
    assert(most_significand_word);
    for (size_t i = sizeof(LibintWord) * CHAR_BIT; i--;) {
        if (most_significand_word & (((LibintWord) 1) << i)) {
            *msb = i + (x->size - 1) * sizeof(LibintWord) * CHAR_BIT;
            break;
        }
    }
end:
    return err;
}

LibintError libint_unsigned_wordshift(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, int offset) {
    LibintError err = LIBINT_ERROR_OK;
    LibintWord *result_ptr = NULL;
    if (!libint || !out || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    size_t result_size;
    if (offset >= 0) {
        result_ptr = malloc((x->size + offset) * sizeof(LibintWord));
        if (!result_ptr) {
            err = LIBINT_ERROR_OUT_OF_MEMORY;
            goto end;
        }
        memcpy(result_ptr + offset, x->ptr, x->size * sizeof(LibintWord));
        memset(result_ptr, 0, offset * sizeof(LibintWord));
    } else if (offset < 0) {
        result_ptr = malloc((x->size + offset) * sizeof(LibintWord));
        if (!result_ptr) {
            err = LIBINT_ERROR_OUT_OF_MEMORY;
            goto end;
        }
        memcpy(result_ptr, x->ptr - offset, (x->size + offset) * sizeof(LibintWord));
    }
    result_size = x->size + offset;
    err = E(libint_unsigned_construct(libint, out, result_size, result_ptr));
    if (err) goto end;
    result_ptr = NULL;
end:
    free(result_ptr);
    return err;
}

LibintError libint_unsigned_bitshift(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, int offset) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !out || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    bool is_zero;
    err = E(libint_unsigned_is_zero(libint, x, &is_zero));
    if (err) goto end;
    if (is_zero || !offset) {
        err = E(libint_unsigned_copy(libint, out, x));
        goto end;
    }
    size_t msb = 0;
    err = E(libint_unsigned_most_significant_bit(libint, x, &msb));
    if (err) goto end;
    if ((int) msb + offset < 0) {
        err = E(libint_unsigned_copy(libint, out, libint->libint_unsigned_constants[0]));
        goto end;
    }
    size_t word_size_in_bits = sizeof(LibintWord) * CHAR_BIT;
    int word_offset = floor((double) (offset + (int) msb % (int) word_size_in_bits) / word_size_in_bits);
    err = E(libint_unsigned_wordshift(libint, &result, x, word_offset));
    if (err) goto end;
    int remaining_offset = offset - word_offset * (int) word_size_in_bits;
    assert(0 <= abs(remaining_offset) && abs(remaining_offset) < (int) word_size_in_bits);
    if (remaining_offset < 0) {
        for (size_t i = 0; i < result->size; ++i) {
            LibintDword hi = (i + 1 < result->size) ? result->ptr[i + 1] : 0;
            LibintDword lo = result->ptr[i];
            LibintDword dword = (hi << word_size_in_bits) | lo;
            result->ptr[i] = dword >> -remaining_offset;
        }
    } else if (0 < remaining_offset) {
        for (size_t i = result->size; i--;) {
            LibintDword hi = result->ptr[i];
            LibintDword lo = i ? result->ptr[i - 1] : 0;
            LibintDword dword = (hi << word_size_in_bits) | lo;
            result->ptr[i] = dword >> (word_size_in_bits - remaining_offset);
        }
    }
    assert(LIBINT_UNSIGNED_INVARIANT(result));
    *out = result;
    result = NULL;
end:
    E(libint_unsigned_destroy(libint, &result));
    return err;
}

LibintError libint_unsigned_mul(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    LibintUnsigned *addend = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    if (x->size < y->size) {
        LibintUnsigned *t = y;
        y = x;
        x = t;
    }
    err = E(libint_unsigned_create(libint, &result, 0));
    if (err) goto end;
    err = E(libint_unsigned_copy(libint, &addend, x));
    if (err) goto end;
    // TODO current implementation is bitwise. it may be much faster if made wordwise.
    for (LibintWord *it = y->ptr, *end = y->ptr + y->size; it != end; ++it) {
        LibintWord word = *it;
        for (LibintWord bit = 0; bit < (LibintWord) sizeof(LibintWord) * CHAR_BIT; ++bit) {
            if (word & (((LibintWord) 1) << bit)) {
                err = E(libint_unsigned_add_replace(libint, &result, addend));
                if (err) goto end;
            }
            err = E(libint_unsigned_bitshift_replace(libint, &addend, 1));
            if (err) goto end;
        }
    }
    *out = result;
    result = NULL;
end:
    E(libint_unsigned_destroy(libint, &addend));
    E(libint_unsigned_destroy(libint, &result));
    return err;
}

LibintError libint_unsigned_div_mod(Libint *libint, LibintUnsigned **out, LibintUnsigned **remainder, LibintUnsigned *x,
                                    LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *window = NULL;
    LibintUnsigned *quotient = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    *remainder = NULL;
    int order;
    err = E(libint_unsigned_compare(libint, y, libint->libint_unsigned_constants[0], &order));
    if (err) goto end;
    if (order == 0) {
        err = LIBINT_ERROR_ARITHMETIC;
        goto end;
    }
    err = E(libint_unsigned_create(libint, &window, 0));
    if (err) goto end;
    err = E(libint_unsigned_create(libint, &quotient, 0));
    if (err) goto end;
    for (LibintWord *x_rit = x->ptr + x->size, *x_rend = x->ptr; x_rit-- != x_rend;) {
        LibintWord word = *x_rit;
        for (size_t bit = sizeof(LibintWord) * CHAR_BIT; bit--;) {
            err = E(libint_unsigned_bitshift_replace(libint, &window, 1));
            if (err) goto end;
            if (word & (((LibintWord) 1) << bit)) {
                err = E(libint_unsigned_add_replace(libint, &window, libint->libint_unsigned_constants[1]));
                if (err) goto end;
            }
            err = E(libint_unsigned_bitshift_replace(libint, &quotient, 1));
            if (err) goto end;
            err = E(libint_unsigned_compare(libint, window, y, &order));
            if (err) goto end;
            if (order >= 0) {
                err = E(libint_unsigned_sub_replace(libint, &window, y));
                if (err) goto end;
                err = E(libint_unsigned_add_replace(libint, &quotient, libint->libint_unsigned_constants[1]));
                if (err) goto end;
            }
        }
    }
    *out = quotient;
    *remainder = window;
    quotient = NULL;
    window = NULL;
end:
    E(libint_unsigned_destroy(libint, &quotient));
    E(libint_unsigned_destroy(libint, &window));
    return err;
}

LibintError libint_unsigned_div(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y) {
    LibintUnsigned *reminder = NULL;
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_div_mod(libint, out, &reminder, x, y));
    if (err) goto end;
end:
    E(libint_unsigned_destroy(libint, &reminder));
    return err;
}

LibintError libint_unsigned_mod(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, LibintUnsigned *y) {
    LibintUnsigned *quotient = NULL;
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_div_mod(libint, &quotient, out, x, y));
    if (err) goto end;
end:
    E(libint_unsigned_destroy(libint, &quotient));
    return err;
}

LibintError libint_unsigned_pow(Libint *libint, LibintUnsigned **out, LibintUnsigned *x, uintmax_t power) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    LibintUnsigned *base = NULL;
    if (!libint || !out || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (!power) {
        err = E(libint_unsigned_copy(libint, out, libint->libint_unsigned_constants[1]));
        goto end;
    }
    *out = NULL;
    err = E(libint_unsigned_copy(libint, &base, x));
    if (err) goto end;
    err = E(libint_unsigned_create(libint, &result, 1));
    if (err) goto end;
    while (power) {
        if (power % 2) {
            err = E(libint_unsigned_mul_replace(libint, &result, base));
            if (err) goto end;
        }
        err = E(libint_unsigned_mul_replace(libint, &base, base));
        if (err) goto end;
        power /= 2;
    }
    *out = result;
    result = NULL;
end:
    E(libint_unsigned_destroy(libint, &base));
    E(libint_unsigned_destroy(libint, &result));
    return err;
}

LibintError libint_unsigned_add_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_add(libint, &result, *x, y));
    if (err) {
        return err;
    }
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_sub_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_sub(libint, &result, *x, y));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_rsub_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    LibintUnsigned *a = y;
    LibintUnsigned *b = *x;
    err = E(libint_unsigned_add(libint, &result, a, b));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_bitshift_replace(Libint *libint, LibintUnsigned **x, int offset) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_bitshift(libint, &result, *x, offset));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_mul_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_mul(libint, &result, *x, y));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_div_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_div(libint, &result, *x, y));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_rdiv_replace(Libint *libint, LibintUnsigned **x, LibintUnsigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    LibintUnsigned *a = y;
    LibintUnsigned *b = *x;
    err = E(libint_unsigned_div(libint, &result, a, b));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_pow_replace(Libint *libint, LibintUnsigned **x, uintmax_t power) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *result = NULL;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_unsigned_pow(libint, &result, *x, power));
    if (err) goto end;
    E(libint_unsigned_destroy(libint, x));
    *x = result;
end:
    return err;
}

LibintError libint_unsigned_is_zero(Libint *libint, LibintUnsigned *x, bool *is_zero) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !is_zero) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    assert(x->size);
    *is_zero = x->size > 1 ? false : !x->ptr[0];
end:
    return err;
}

LibintError libint_unsigned_compare(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, int *order) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !order) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (x->size < y->size) {
        *order = -1;
        goto end;
    }
    if (x->size > y->size) {
        *order = 1;
        goto end;
    }
    *order = 0;
    for (LibintWord *x_rit = x->ptr + x->size,
                 *x_rend = x->ptr,
                 *y_rit = y->ptr + y->size;
         --y_rit, x_rit-- != x_rend;) {
        LibintWord a = *x_rit;
        LibintWord b = *y_rit;
        if (a < b) {
            *order = -1;
            break;
        }
        if (a > b) {
            *order = 1;
            break;
        }
    }
end:
    return err;
}

LibintError libint_unsigned_less(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, bool *out) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !out) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    int order;
    err = E(libint_unsigned_compare(libint, x, y, &order));
    if (err) {
        return err;
    }
    *out = order < 0;
end:
    return err;
}

LibintError libint_unsigned_less_or_equal(Libint *libint, LibintUnsigned *x, LibintUnsigned *y, bool *out) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !out) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    int order;
    err = E(libint_unsigned_compare(libint, x, y, &order));
    if (err) goto end;
    *out = order <= 0;
end:
    return LIBINT_ERROR_OK;
}
