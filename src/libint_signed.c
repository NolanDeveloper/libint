#include "libint_internal.h"

#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

LibintError libint_handle_internal_error(LibintError err) {
    switch (err) {
    case LIBINT_ERROR_OK:
        return LIBINT_ERROR_OK;
    case LIBINT_ERROR_OUT_OF_MEMORY:
        return LIBINT_ERROR_OUT_OF_MEMORY;
    case LIBINT_ERROR_BAD_ARGUMENT:
        abort();
    case LIBINT_ERROR_ARITHMETIC:
        return LIBINT_ERROR_ARITHMETIC;
    case LIBINT_ERROR_PARSE:
        return LIBINT_ERROR_PARSE;
    case LIBINT_ERROR_IO:
        return LIBINT_ERROR_IO;
    }
    abort();
}

LibintError libint_start(Libint **libint) {
    LibintError err = LIBINT_ERROR_OK;
    Libint *result = NULL;
    intmax_t i = 0;
    intmax_t j = 0;
    if (!libint) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(Libint));
    if (!result) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    intmax_t n = sizeof(result->libint_unsigned_constants) / sizeof(LibintUnsigned *);
    for (; i < n; ++i) {
        err = E(libint_unsigned_create(result, &result->libint_unsigned_constants[i], i));
        if (err) goto end;
    }
    n = sizeof(result->libint_constants) / sizeof(LibintSigned *);
    for (; j < n; ++j) {
        err = E(libint_create(result, &result->libint_constants[j], j));
        if (err) goto end;
    }
    err = EIS(libis_start(&result->libis));
    if (err) goto end;
    *libint = result;
    i = 0;
    j = 0;
    result = NULL;
end:
    if (result) {
        for (size_t t = j; t--;) {
            E(libint_destroy(result, &result->libint_constants[t]));
        }
        for (size_t t = i; t--;) {
            E(libint_unsigned_destroy(result, &result->libint_unsigned_constants[t]));
        }
        free(result);
    }
    return err;
}

LibintError libint_finish(Libint **libint) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*libint) {
        size_t n = sizeof((*libint)->libint_unsigned_constants) / sizeof(LibintUnsigned *);
        for (size_t i = 0; i < n; ++i) {
            E(libint_unsigned_destroy(*libint, &(*libint)->libint_unsigned_constants[i]));
        }
        n = sizeof((*libint)->libint_constants) / sizeof(LibintSigned *);
        for (size_t i = 0; i < n; ++i) {
            E(libint_destroy(*libint, &(*libint)->libint_constants[i]));
        }
        EIS(libis_finish(&(*libint)->libis));
        free(*libint);
        *libint = NULL;
    }
end:
    return err;
}

static void normalize(LibintSigned *x) {
    if (!x || !x->magnitude) {
        abort();
    }
    if (x->magnitude->size > 1) {
        return;
    }
    if (x->magnitude->ptr[0]) {
        return;
    }
    x->is_negative = false;
}

LibintError libint_create(Libint *libint, LibintSigned **x, intmax_t value) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *magnitude = NULL;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    bool is_negative = value < 0;
    if (is_negative) {
        value = -value;
    }
    err = E(libint_unsigned_create(libint, &magnitude, (uintmax_t) value));
    if (err) goto end;
    err = E(libint_construct(libint, x, is_negative, magnitude));
    if (err) goto end;
    magnitude = NULL;
end:
    E(libint_unsigned_destroy(libint, &magnitude));
    return err;
}

LibintError libint_to_intmax(Libint *libint, LibintSigned *x, intmax_t *value) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !value) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = 0;
    uintmax_t magnitude_value;
    err = E(libint_unsigned_to_uintmax(libint, x->magnitude, &magnitude_value));
    if (err) goto end;
    if (x->is_negative) {
        if (magnitude_value > -(uintmax_t) INTMAX_MIN) {
            err = LIBINT_ERROR_ARITHMETIC;
            goto end;
        }
        *value = -(uintmax_t) magnitude_value;
    } else {
        if (magnitude_value > (uintmax_t) INTMAX_MAX) {
            err = LIBINT_ERROR_ARITHMETIC;
            goto end;
        }
        *value = magnitude_value;
    }
end:
    return err;
}

LibintError libint_from_string(Libint *libint, LibintSigned **x, const char *input, size_t input_size, int base) {
    LibintError err = LIBINT_ERROR_OK;
    LibisInputStream *input_stream = NULL;
    LibisSource *source = NULL;
    if (!libint || !x || !input_size) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = EIS(libis_source_create_from_buffer(libint->libis, &source, input, input_size, false));
    if (err) goto end;
    err = EIS(libis_create(libint->libis, &input_stream, &source, 1));
    if (err) goto end;
    err = E(libint_from_input_stream(libint, x, input_stream, base));
    if (err) goto end;
end:
    EIS(libis_source_destroy(libint->libis, &source));
    EIS(libis_destroy(libint->libis, &input_stream));
    return err;
}

LibintError libint_from_input_stream(Libint *libint, LibintSigned **x, LibisInputStream *input, int base) {
    LibintError err = LIBINT_ERROR_OK;
    char c;
    bool eof;
    LibintUnsigned *magnitude = NULL;
    if (!libint || !x || base < 2 || 16 < base || !input) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_skip_whitespace(libint, input, &eof, &c));
    if (err) goto end;
    bool is_negative = false;
    if ('-' == c || '+' == c) {
        is_negative = '-' == c;
        err = EIS(libis_skip_char(libint->libis, input, &eof, &c));
        if (err) goto end;
    }
    err = E(libint_unsigned_from_input_stream(libint, &magnitude, input, base));
    if (err) goto end;
    err = E(libint_construct(libint, x, is_negative, magnitude));
    if (err) goto end;
    magnitude = NULL;
end:
    E(libint_unsigned_destroy(libint, &magnitude));
    return err;
}

LibintError libint_to_string(Libint *libint, LibintSigned *x, int base, char **out, size_t *out_size) {
    if (!libint || !x || !out || !out_size) {
        return LIBINT_ERROR_BAD_ARGUMENT;
    }
    return E(libint_to_string_helper(libint, x->is_negative, x->magnitude, base, out, out_size));
}

LibintError libint_construct(Libint *libint, LibintSigned **x, bool is_negative, LibintUnsigned *magnitude) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !magnitude) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *x = NULL;
    LibintSigned *out = malloc(sizeof(LibintSigned));
    if (!out) {
        err = LIBINT_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    out->is_negative = is_negative;
    out->magnitude = magnitude;
    normalize(out);
    assert(LIBINT_SIGNED_INVARIANT(out));
    *x = out;
end:
    return err;
}

LibintError libint_copy(Libint *libint, LibintSigned **out, LibintSigned *x) {
    if (!libint || !out || !x) {
        return LIBINT_ERROR_BAD_ARGUMENT;
    }
    return E(libint_add(libint, out, libint->libint_constants[0], x));
}

LibintError libint_destroy(Libint *libint, LibintSigned **x) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*x) {
        E(libint_unsigned_destroy(libint, &(*x)->magnitude));
        free(*x);
        *x = NULL;
    }
end:
    return err;
}

LibintError libint_add(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *out_magnitude = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    if (x->is_negative == y->is_negative) {
        err = E(libint_unsigned_add(libint, &out_magnitude, x->magnitude, y->magnitude));
        if (err) goto end;
        err = E(libint_construct(libint, out, x->is_negative, out_magnitude));
        if (err) goto end;
    } else {
        int order;
        err = E(libint_unsigned_compare(libint, x->magnitude, y->magnitude, &order));
        if (err) goto end;
        bool is_negative = x->is_negative;
        if (order < 0) {
            LibintSigned *t = x;
            x = y;
            y = t;
            is_negative = !is_negative;
        }
        err = E(libint_unsigned_sub(libint, &out_magnitude, x->magnitude, y->magnitude));
        if (err) goto end;
        err = E(libint_construct(libint, out, is_negative, out_magnitude));
        if (err) goto end;
    }
    out_magnitude = NULL;
end:
    E(libint_unsigned_destroy(libint, &out_magnitude));
    return err;
}

LibintError libint_sub(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    y->is_negative = !y->is_negative;
    err = E(libint_add(libint, out, x, y));
    y->is_negative = !y->is_negative;
    if (err) goto end;
end:
    return err;
}

LibintError libint_mul(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *out_magnitude = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    bool is_negative = x->is_negative != y->is_negative;
    err = E(libint_unsigned_mul(libint, &out_magnitude, x->magnitude, y->magnitude));
    if (err) goto end;
    err = E(libint_construct(libint, out, is_negative, out_magnitude));
    if (err) goto end;
    out_magnitude = NULL;
end:
    E(libint_unsigned_destroy(libint, &out_magnitude));
    return err;
}

LibintError libint_div_trunc(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *remainder = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_div_mod_trunc(libint, out, &remainder, x, y));
    if (err) goto end;
end:
    E(libint_destroy(libint, &remainder));
    return err;
}

LibintError libint_mod_trunc(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *quotient = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_div_mod_trunc(libint, &quotient, out, x, y));
    if (err) goto end;
end:
    E(libint_destroy(libint, &quotient));
    return err;
}

LibintError libint_div_mod_trunc(Libint *libint, LibintSigned **quotient, LibintSigned **remainder, LibintSigned *x,
                                 LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintUnsigned *quotient_magnitude = NULL;
    LibintUnsigned *remainder_magnitude = NULL;
    if (!libint || !quotient || !remainder || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    bool is_negative_quotient = x->is_negative != y->is_negative;
    bool is_negative_remainder = x->is_negative;
    err = E(libint_unsigned_div_mod(libint, &quotient_magnitude, &remainder_magnitude, x->magnitude, y->magnitude));
    if (err) goto end;
    err = E(libint_construct(libint, quotient, is_negative_quotient, quotient_magnitude));
    if (err) goto end;
    quotient_magnitude = NULL;
    err = E(libint_construct(libint, remainder, is_negative_remainder, remainder_magnitude));
    if (err) goto end;
    remainder_magnitude = NULL;
end:
    E(libint_unsigned_destroy(libint, &quotient_magnitude));
    E(libint_unsigned_destroy(libint, &remainder_magnitude));
    return err;
}

LibintError libint_div_floor(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    LibintUnsigned *result_magnitude = NULL;
    LibintUnsigned *remainder = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = NULL;
    bool is_negative_quotient = x->is_negative != y->is_negative;
    err = E(libint_unsigned_div_mod(libint, &result_magnitude, &remainder, x->magnitude, y->magnitude));
    if (err) goto end;
    err = E(libint_construct(libint, &result, is_negative_quotient, result_magnitude));
    if (err) goto end;
    result_magnitude = NULL;
    int order = 0;
    err = E(libint_unsigned_compare(libint, remainder, libint->libint_unsigned_constants[0], &order));
    if (err) goto end;
    if (is_negative_quotient && order == 1) {
        err = E(libint_sub_replace(libint, &result, libint->libint_constants[1]));
        if (err) goto end;
    }
    *out = result;
    result = NULL;
end:
    E(libint_unsigned_destroy(libint, &result_magnitude));
    E(libint_unsigned_destroy(libint, &remainder));
    E(libint_destroy(libint, &result));
    return err;
}

// x % y = x - y * floor(x / y)
LibintError libint_mod_floor(Libint *libint, LibintSigned **out, LibintSigned *x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !out || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_div_floor(libint, &result, x, y));
    if (err) goto end;
    err = E(libint_mul_replace(libint, &result, y));
    if (err) goto end;
    err = E(libint_rsub_replace(libint, &result, x));
    if (err) goto end;
    *out = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return err;
}

LibintError libint_div_mod_floor(Libint *libint, LibintSigned **quotient, LibintSigned **remainder, LibintSigned *x,
                                 LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result_quotient = NULL;
    LibintSigned *result_remainder = NULL;
    if (!libint || !quotient || !remainder || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_div_floor(libint, &result_quotient, x, y));
    if (err) goto end;
    err = E(libint_mod_floor(libint, &result_remainder, x, y));
    if (err) goto end;
    *quotient = result_quotient;
    result_quotient = NULL;
    *remainder = result_remainder;
    result_remainder = NULL;
end:
    E(libint_destroy(libint, &result_quotient));
    E(libint_destroy(libint, &result_remainder));
    return err;
}

LibintError libint_add_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_add(libint, &result, *x, y));
    if (err) goto end;
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return err;
}

LibintError libint_sub_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_sub(libint, &result, *x, y));
    if (err) goto end;
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return err;
}

LibintError libint_rsub_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    LibintSigned *a = y;
    LibintSigned *b = *x;
    err = E(libint_sub(libint, &result, a, b));
    if (err) goto end;
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return err;
}

LibintError libint_mul_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_mul(libint, &result, *x, y));
    if (err) goto end;
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return LIBINT_ERROR_OK;
}

LibintError libint_div_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result = NULL;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libint_div_trunc(libint, &result, *x, y));
    if (err) {
        return err;
    }
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return LIBINT_ERROR_OK;
}

LibintError libint_rdiv_replace(Libint *libint, LibintSigned **x, LibintSigned *y) {
    LibintError err = LIBINT_ERROR_OK;
    LibintSigned *result;
    if (!libint || !x || !y) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    LibintSigned *a = y;
    LibintSigned *b = *x;
    err = E(libint_div_trunc(libint, &result, a, b));
    if (err) goto end;
    E(libint_destroy(libint, x));
    *x = result;
    result = NULL;
end:
    E(libint_destroy(libint, &result));
    return LIBINT_ERROR_OK;
}

LibintError libint_is_zero(Libint *libint, LibintSigned *x, bool *is_zero) {
    if (!libint || !x || !is_zero) {
        return LIBINT_ERROR_BAD_ARGUMENT;
    }
    return E(libint_unsigned_is_zero(libint, x->magnitude, is_zero));
}

LibintError libint_compare(Libint *libint, LibintSigned *x, LibintSigned *y, int *order) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !order) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (x->is_negative) {
        if (y->is_negative) {
            err = E(libint_unsigned_compare(libint, y->magnitude, x->magnitude, order));
            if (err) goto end;
        } else {
            *order = -1;
        }
    } else {
        if (y->is_negative) {
            *order = 1;
        } else {
            err = E(libint_unsigned_compare(libint, x->magnitude, y->magnitude, order));
            if (err) goto end;
        }
    }
end:
    return err;
}

LibintError libint_less(Libint *libint, LibintSigned *x, LibintSigned *y, bool *out) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !out) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    int order;
    err = E(libint_compare(libint, x, y, &order));
    if (err) goto end;
    *out = order < 0;
end:
    return err;
}

LibintError libint_less_or_equal(Libint *libint, LibintSigned *x, LibintSigned *y, bool *out) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !x || !y || !out) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    int order;
    err = libint_compare(libint, x, y, &order);
    if (err) goto end;
    *out = order <= 0;
end:
    return err;
}
