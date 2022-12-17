#include <libint.h>

#include <math.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>

static Libint *libint;

static int rand_between(int a, int b) {
    assert(a < b);
    return a + rand() % (b - a); // NOLINT(cert-msc50-cpp)
}

static void test_shl(uintmax_t a) {
    LibintError err;

    LibintUnsigned *x;
    err = libint_unsigned_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    int uintmax_bit_size = sizeof(uintmax_t) * CHAR_BIT;
    int offset = rand_between(-(uintmax_bit_size / 2 - 1), (uintmax_bit_size / 2 - 1));

    err = libint_unsigned_bitshift_replace(libint, &x, offset);
    assert(LIBINT_ERROR_OK == err);

    LibintUnsigned *expected;
    err = libint_unsigned_create(libint, &expected, (offset < 0) ? (a >> -offset) : (a << offset));
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_unsigned_compare(libint, x, expected, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_unsigned_destroy(libint, &x);
    libint_unsigned_destroy(libint, &expected);
}

static void test_copy(intmax_t a) {
    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *x_copy;
    err = libint_copy(libint, &x_copy, x);
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, x, x_copy, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_destroy(libint, &x);
    libint_destroy(libint, &x_copy);
}

static void test_add(intmax_t a, intmax_t b) {
    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *y;
    err = libint_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *sum;
    err = libint_add(libint, &sum, x, y);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *expected_sum;
    err = libint_create(libint, &expected_sum, a + b);
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, sum, expected_sum, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_destroy(libint, &x);
    libint_destroy(libint, &y);
    libint_destroy(libint, &sum);
    libint_destroy(libint, &expected_sum);
}

void test_mul(intmax_t a, intmax_t b) {
    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *y;
    err = libint_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *product;
    err = libint_mul(libint, &product, x, y);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *expected_product;
    err = libint_create(libint, &expected_product, a * b);
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, product, expected_product, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_destroy(libint, &x);
    libint_destroy(libint, &y);
    libint_destroy(libint, &product);
    libint_destroy(libint, &expected_product);
}

void test_replace(intmax_t a, intmax_t b,
                  LibintError (*big_operation)(Libint *, LibintSigned **, LibintSigned *),
                  intmax_t (*small_operation)(intmax_t, intmax_t)) {
    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *y;
    err = libint_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    err = big_operation(libint, &x, y);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *expected_result;
    err = libint_create(libint, &expected_result, small_operation(a, b));
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, x, expected_result, &order);
    assert(LIBINT_ERROR_OK == err);

    libint_destroy(libint, &x);
    libint_destroy(libint, &y);
    libint_destroy(libint, &expected_result);
}

void test_unsigned_replace(intmax_t a, intmax_t b,
                           LibintError (*big_operation)(Libint *, LibintUnsigned **, LibintUnsigned *),
                           intmax_t (*small_operation)(intmax_t, intmax_t)) {
    LibintError err;

    LibintUnsigned *x;
    err = libint_unsigned_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintUnsigned *y;
    err = libint_unsigned_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    err = big_operation(libint, &x, y);
    assert(LIBINT_ERROR_OK == err);

    LibintUnsigned *expected_result;
    err = libint_unsigned_create(libint, &expected_result, small_operation(a, b));
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_unsigned_compare(libint, x, expected_result, &order);
    assert(LIBINT_ERROR_OK == err);

    libint_unsigned_destroy(libint, &x);
    libint_unsigned_destroy(libint, &y);
    libint_unsigned_destroy(libint, &expected_result);
}

intmax_t imax_add(intmax_t a, intmax_t b) {
    return a + b;
}

intmax_t imax_sub(intmax_t a, intmax_t b) {
    return a - b;
}

intmax_t imax_rsub(intmax_t a, intmax_t b) {
    return b - a;
}

intmax_t imax_mul(intmax_t a, intmax_t b) {
    return a * b;
}

intmax_t imax_div(intmax_t a, intmax_t b) {
    return a / b;
}

intmax_t imax_rdiv(intmax_t a, intmax_t b) {
    return b / a;
}

void test_div_trunc(int a, int b) {
    assert(b);

    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *y;
    err = libint_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *quotient, *remainder;
    err = libint_div_mod_trunc(libint, &quotient, &remainder, x, y);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *expected_quotient;
    err = libint_create(libint, &expected_quotient, trunc((double) a / (double) b));
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, quotient, expected_quotient, &order);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *expected_remainder;
    err = libint_create(libint, &expected_remainder, trunc(fmod((double) a, (double) b)));
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    err = libint_compare(libint, remainder, expected_remainder, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_destroy(libint, &x);
    libint_destroy(libint, &y);
    libint_destroy(libint, &quotient);
    libint_destroy(libint, &remainder);
    libint_destroy(libint, &expected_quotient);
    libint_destroy(libint, &expected_remainder);
}

void test_div_floor(intmax_t a, intmax_t b) {
    assert(b);

    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *y;
    err = libint_create(libint, &y, b);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *q, *r;
    err = libint_div_mod_floor(libint, &q, &r, x, y);
    assert(LIBINT_ERROR_OK == err);

    intmax_t expected_quotient_intmax = floor((double) a / (double) b);
    LibintSigned *expected_quotient;
    err = libint_create(libint, &expected_quotient, expected_quotient_intmax);
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, q, expected_quotient, &order);
    assert(LIBINT_ERROR_OK == err);

    intmax_t expected_remainder_intmax = a - expected_quotient_intmax * b;
    LibintSigned *expected_remainder;
    err = libint_create(libint, &expected_remainder, expected_remainder_intmax);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    err = libint_compare(libint, r, expected_remainder, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    libint_destroy(libint, &x);
    libint_destroy(libint, &y);
    libint_destroy(libint, &q);
    libint_destroy(libint, &r);
    libint_destroy(libint, &expected_quotient);
    libint_destroy(libint, &expected_remainder);
}

extern char *int_to_string(LibintSigned *value, int base) {
    char *out;
    size_t out_size;
    LibintError err = libint_to_string(libint, value, base, &out, &out_size);
    assert(LIBINT_ERROR_OK == err);
    return out;
}

void test_str(intmax_t a) {
    int base = 2 + rand() % (16 - 2);

    LibintError err;

    LibintSigned *x;
    err = libint_create(libint, &x, a);
    assert(LIBINT_ERROR_OK == err);

    char *str;
    size_t str_size;
    err = libint_to_string(libint, x, base, &str, &str_size);
    assert(LIBINT_ERROR_OK == err);

    LibintSigned *parsed;
    const char *end_of_input = NULL;
    err = libint_from_string(libint, &parsed, str, str_size, base, &end_of_input);
    assert(LIBINT_ERROR_OK == err);

    int order;
    err = libint_compare(libint, x, parsed, &order);
    assert(LIBINT_ERROR_OK == err);

    assert(!order);

    free(str);
    libint_destroy(libint, &x);
    libint_destroy(libint, &parsed);
}

void test(void) {
    LibintError err = libint_start(&libint);
    assert(LIBINT_ERROR_OK == err);

    srand(42);
    for (int i = 0; i < 1000; ++i) {
        intmax_t a = (rand() % 10000) - 5000;
        intmax_t b = (rand() % 10000) - 5000;

        test_str(a);
        test_shl(imaxabs(a));
        test_copy(a);
        test_add(a, b);
        test_mul(a, b);
        if (b != 0) {
            test_div_floor(a, b);
            test_div_trunc(a, b);
        }
        test_replace(a, b, libint_add_replace, imax_add);
        test_replace(a, b, libint_sub_replace, imax_sub);
        test_replace(a, b, libint_rsub_replace, imax_rsub);
        test_replace(a, b, libint_mul_replace, imax_mul);
        a = imaxabs(a);
        b = imaxabs(b);
        if (b > a) {
            intmax_t t = a;
            a = b;
            b = t;
        }
        test_unsigned_replace(a, b, libint_unsigned_add_replace, imax_add);
        test_unsigned_replace(a, b, libint_unsigned_sub_replace, imax_sub);
        test_unsigned_replace(a, b, libint_unsigned_rsub_replace, imax_rsub);
        test_unsigned_replace(a, b, libint_unsigned_mul_replace, imax_mul);
        if (b != 0) {
            test_unsigned_replace(a, b, libint_unsigned_div_replace, imax_div);
        }
        if (a != 0) {
            test_unsigned_replace(a, b, libint_unsigned_rdiv_replace, imax_rdiv);
        }
    }

    libint_finish(&libint);
}

int main() {
    test();
    return EXIT_SUCCESS;
}
