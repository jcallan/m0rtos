/*
 * float32.c
 *
 *  Created on: May 29, 2020
 *      Author: Jcallan
 *
 * This file contains routines for handling floating point numbers with 32 bits
 * of mantissa, 8 bits of exponent and a sign bit
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>         /* For strchr */
#include "float32.h"

#define INCLUDE_F32_TESTS       1

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

const f32_t plus_zero      = {0UL, INT8_MIN, +1};
const f32_t minus_zero     = {0UL, INT8_MIN, -1};
const f32_t plus_infinity  = {INT32_MAX, INT8_MAX, +1};
const f32_t minus_infinity = {INT32_MAX, INT8_MAX, -1};
const f32_t pi             = {3373259426UL, -30, +1};
const f32_t half_pi        = {3373259426UL, -31, +1};
const f32_t quarter_pi     = {3373259426UL, -32, +1};
const f32_t two_pi         = {3373259426UL, -29, +1};
const f32_t one_third_pi   = {2248839617UL, -31, +1};
const f32_t two_thirds_pi  = {2248839617UL, -30, +1};
const f32_t one_sixth_pi   = {2248839617UL, -32, +1};
const f32_t root_2         = {3037000500UL, -31, +1};
const f32_t half_root_2    = {3037000500UL, -32, +1};
const f32_t root_3         = {3719550787UL, -31, +1};
const f32_t half_root_3    = {3719550787UL, -32, +1};

int _print_integer(char *buffer, uint32_t integer, bool plus_sign, bool negative)
{
    static const uint32_t place_val[10] = {1000000000, 100000000, 10000000, 1000000, 100000, 10000,
                                           1000, 100, 10, 1};
    uint32_t digit;
    int i, len = 0;
    bool started = false;

    if (negative)
    {
        buffer[len] = '-';
        ++len;
    }
    else
    {
        if (plus_sign)
        {
            buffer[len] = '+';
            ++len;
        }
    }
            
    for (i = 0; i < 10; ++i)
    {
        digit = integer / place_val[i];
        if (started || (digit > 0) || (i == 9))
        {
            integer -= digit * place_val[i];
            buffer[len] = '0' + digit;
            ++len;
            started = true;
        }
    }
    
    return len;
}

int _print_fraction(char *buffer, uint32_t fraction, int decimal_places, bool zeroes)
{
    static const uint32_t point[10] = {0, 429496729u, 858993459u, 1288490188u, 1717986918u,
                                       2147483648u, 2576980377u, 3006477107u, 3435973836u,
                                       3865470566u};
    unsigned place;
    int i, len = 0;
                                       
    for (place = 0; place < decimal_places; ++place)
    {
        if (fraction == 0 && !zeroes)
        {
            break;
        }
        for (i = 9; i >= 0; --i)
        {
            if (fraction >= point[i])
            {
                fraction -= point[i];
                buffer[len] = '0' + i;
                ++len;
                break;
            }
        }
        fraction *= 10;
    }
    
    return len;
}

int sprint_f32(char *buffer, const f32_t *a, int decimal_places, bool plus, bool zeroes)
{
    int exponent, len = 0;
    uint32_t integer, mantissa;
    uint32_t fraction;
    bool scientific, negative;

    if (a->mantissa != 0 && (a->exponent > 0 || a->exponent < -64))
    {
        /* Big/small number, print in exponent notation */
        scientific = true;
        integer = 1;
        fraction = a->mantissa << 1;
    }
    else
    {
        /* Normal size number, print in decimal */
        scientific = false;
        mantissa = a->mantissa;
        integer  = mantissa >> -a->exponent;
        if (a->exponent >= -32)
        {
            fraction = mantissa << (32 + a->exponent);
        }
        else
        {
            fraction = mantissa >> (-32 - a->exponent);
        }
    }

    len += _print_integer(buffer, integer, plus, a->signum < 0);
    
    if (decimal_places > 0 && (fraction != 0 || zeroes))
    {
        buffer[len] = '.';
        ++len;
        len += _print_fraction(buffer + len, fraction, decimal_places, zeroes);
    }

    if (scientific)
    {
        buffer[len] = 'b';
        ++len;
        exponent = a->exponent + 31;
        if (exponent < 0)
        {
            negative = true;
            exponent = -exponent;
        }
        len += _print_integer(buffer + len, exponent, true, negative);
    }
    
    buffer[len] = 0;
    return len;
}

int _get_integer(const char *buffer, uint32_t *integer)
{
    int len = 0, total = 0;
    char digit;
    
    while (len < 9)
    {
        digit = buffer[len];
        if (digit >= '0' && digit <= '9')
        {
            total *= 10;
            total += digit - '0';
        }
        else
        {
            break;
        }
        ++len;
    }
    
    *integer = total;
    return len;
}

int parse_f32(f32_t *ret, const char *buffer)
{
    int consumed;
    uint32_t integer_part, fraction_part = 0;
    uint32_t fraction_divider = 1;
    int signum = 1, decimal_len = 0;
    
    consumed = 0;
    while (buffer[consumed] == ' ' || buffer[consumed] == '\t')
    {
        ++consumed;
    }
    if (buffer[consumed] == '+')
    {
        ++consumed;
    }
    if (buffer[consumed] == '-')
    {
        ++consumed;
        signum = -1;
    }
    consumed += _get_integer(&buffer[consumed], &integer_part);
    if ((consumed > 0) && (buffer[consumed] == '.'))
    {
        ++consumed;
        decimal_len = _get_integer(&buffer[consumed], &fraction_part);
        consumed += decimal_len;
    }

    if (decimal_len > 0 && decimal_len <= 9)
    {
        /* Find divisor for fraction part */
        while (decimal_len--)
        {
            fraction_divider *= 10;
        }
    }

    /* Add the integer and fraction parts together */
    make_f32(ret, fraction_part, 0);
    idivide_f32(ret, ret, fraction_divider);
    iadd_f32(ret, ret, integer_part);
    ret->signum = signum;
    
    return consumed;
}

/*
 * Turn a float into an f32_t
 *
 * Not all floats are representable as f32_t:
 *   exponent < 29  (i.e float smaller than 2^-98)
 *   infinity
 *   NaN
 *   subnormal numbers
 */
void get_f32_from_float(f32_t *a, float f)
{
    uint32_t val;
    int exponent;

    /* Read the 32 bits of the float into val */
    val = *(uint32_t *)&f;
    
    /* Now pick out the mantissa, exponent and sign */
    a->mantissa = ((val & 0x007fffffu) <<  8) | 0x80000000u;
    exponent    = ((val & 0x7f800000u) >> 23);
    a->signum   =  (val & 0x80000000u) ? -1 : +1;
    
    if (exponent == 0)
    {
        a->exponent = INT8_MIN;
        a->mantissa = 0;
    }
    else
    {
        a->exponent =  exponent - 158;
    }
}

/*
 * Calculate by how much we could shift an unt32_t without overflow
 */
static int count_leading_space(uint32_t val)
{
    int space = 0;
    if (val == 0)
    {
        return 32;
    }
    if (val < (1u << 16))
    {
        val <<= 16;
        space += 16;
    }
    if (val < (1u << 24))
    {
        val <<= 8;
        space += 8;
    }
    if (val < (1u << 28))
    {
        val <<= 4;
        space += 4;
    }
    if (val < (1u << 30))
    {
        val <<= 2;
        space += 2;
    }
    if (val < (1u << 31))
    {
        space += 1;
    }
    
    return space;
}

void normalise_f32(f32_t *a)
{
    int shift, new_exponent;
    
    /* Zero is a kind of denormal value */
    if (a->mantissa == 0)
    {
        a->exponent = INT8_MIN;
        return;
    }
    shift = count_leading_space(a->mantissa);
    new_exponent = a->exponent - shift;
    if (new_exponent < INT8_MIN)
    {
        shift -= new_exponent - INT8_MIN;
    }
    a->mantissa <<= shift;
    a->exponent -= shift;
    
    return;
}

void multiply_f32(f32_t *ret, const f32_t *a, const f32_t *b)
{
    uint32_t mantissa;
    int exponent, signum;

    mantissa = ((uint64_t)a->mantissa * b->mantissa) >> 32;
    exponent = a->exponent + b->exponent + 32;
    signum   = (a->signum == b->signum ? 1 : -1);
    
    /* Check for overflow/underflow and saturate appropriately */
    if (exponent > INT8_MAX)
    {
        exponent = INT8_MAX;
        mantissa = UINT32_MAX;
    }
    if (exponent < INT8_MIN)
    {
        exponent = INT8_MIN;
        mantissa = 0;
    }

    ret->mantissa = mantissa;
    ret->exponent = exponent;
    ret->signum   = signum;
    normalise_f32(ret);
}

void imultiply_f32(f32_t *ret, const f32_t *a, int32_t b)
{
    f32_t bb;

    make_f32(&bb, b, 0);
    normalise_f32(&bb);
    multiply_f32(ret, a, &bb);
}

void divide_f32(f32_t *ret, const f32_t *a, const f32_t *b)
{
    uint32_t numerator, denominator, answer, current_bit;
    int exponent, i;
    
    numerator   = a->mantissa;
    denominator = b->mantissa;
    exponent    = a->exponent - b->exponent - 31;

    /* Determine the sign of the result */
    if (a->signum == b->signum)
    {
        ret->signum = 1;
    }
    else
    {
        ret->signum = -1;
    }

    /* Check for zeroes in either numerator or denominator */
    if (numerator == 0)
    {
        ret->exponent = INT8_MIN;
        ret->mantissa = 0;
        return;
    }
    if (denominator == 0)
    {
        ret->exponent = INT8_MAX;
        ret->mantissa = UINT32_MAX;
        return;
    }
    
    /* We want the denominator as big as possible, but not bigger than the numerator */
    if (denominator > numerator)
    {
        denominator >>= 1;
        --exponent;
    }
    
    /* Check for overflow/underflow and saturate appropriately */
    if (exponent > INT8_MAX)
    {
        ret->mantissa = UINT32_MAX;
        ret->exponent = INT8_MAX;
        return;
    }
    if (exponent < INT8_MIN)
    {
        ret->mantissa = 0;
        ret->exponent = INT8_MIN;
        return;
    }
    
    /*
     * Do the long division
     *
     * This can be made slightly more accurate by using 64 bit values, e.g:
     *     uint64_t numerator64   = ((uint64_t)numerator)   << 32;
     *     uint64_t denominator64 = ((uint64_t)denominator) << 32;
     * But this costs quite a lot more cycles.
     */
    answer = 0;
    current_bit = 0x80000000u;
    
    for (i = 31; i >=0; --i)
    {
        if (numerator >= denominator)
        {
            answer |= current_bit;
            numerator -= denominator;
        }
        denominator >>= 1;
        current_bit >>= 1;
    }

    ret->mantissa = answer;
    ret->exponent = exponent;
}

void idivide_f32(f32_t *ret, const f32_t *a, int32_t b)
{
    f32_t bb;
    make_f32(&bb, b, 0);
    divide_f32(ret, a, &bb);
}

static void _add_or_subtract_f32(f32_t *ret, const f32_t *a, const f32_t *b, int subtract_signum)
{
    int shift_down, exponent, signum1, signum2;
    uint32_t mantissa1, mantissa2, mantissa;
    
    /*
     * Work out which of a and b is absolutely larger. We want to do an 
     * unsigned addition or subtraction of the form
     *   answer = value1 +/- value2
     * where we know that value1 is larger
     */
    if (a->exponent > b->exponent || (a->exponent == b->exponent && a->mantissa >= b->mantissa))
    {
        /* a is bigger, use a as value1 and b as value2 */
        mantissa1  = a->mantissa;
        mantissa2  = b->mantissa;
        exponent   = a->exponent;
        signum1    = a->signum;
        signum2    = b->signum * subtract_signum;
        shift_down = a->exponent - b->exponent;
    }
    else
    {
        /* b is bigger, use b as value1 and a as value2 */
        mantissa1  = b->mantissa;
        mantissa2  = a->mantissa;
        exponent   = b->exponent;
        signum1    = b->signum * subtract_signum;
        signum2    = a->signum;
        shift_down = b->exponent - a->exponent;
    }
    if (shift_down >= 32)
    {
        mantissa2 = 0;
    }
    else
    {
        mantissa2 >>= shift_down;
    }
    
    /* Add or subtract as required */
    if (signum1 == signum2)
    {
        mantissa = (mantissa1 >> 1) + (mantissa2 >> 1); /* TODO use carry bit */
    }
    else
    {
        mantissa = (mantissa1 >> 1) - (mantissa2 >> 1);
    }
    
    /* Normalise the answer */
    ++exponent;                         /* We already halved the answer above */
    if (mantissa > 0)
    {
        while ((mantissa & 0x80000000) == 0)
        {
            mantissa <<= 1;
            --exponent;
        }
    }
    else
    {
        exponent = INT8_MIN;
    }
        
    ret->mantissa = mantissa;
    ret->exponent = exponent;
    ret->signum   = signum1;
}

void add_f32(f32_t *ret, const f32_t *a, const f32_t *b)
{
    _add_or_subtract_f32(ret, a, b, 1);
}

void subtract_f32(f32_t *ret, const f32_t *a, const f32_t *b)
{
    _add_or_subtract_f32(ret, a, b, -1);
}

void iadd_f32(f32_t *ret, const f32_t *a, int32_t b)
{
    f32_t bb;
    make_f32(&bb, b, 0);
    add_f32(ret, a, &bb);
}

/* 
 * Uses Taylor expansion: cosine(x) = 1 - x^2/2! + x^4/4! - x^6/6! + ...
 * We assume that -pi/2 <= a <= pi/2
 */
void cosine_f32(f32_t *ret, const f32_t *a)
{
    f32_t x2, x4, x6, x8, x10;

    /* Calculate the powers of x we need - will always be non-negative */
    multiply_f32(&x2,  a,   a  );
    multiply_f32(&x4,  &x2, &x2);
    multiply_f32(&x6,  &x4, &x2);
    multiply_f32(&x8,  &x4, &x4);
    multiply_f32(&x10, &x4, &x6);

    /* Divide each term by the relevant factorial */
    /* TODO: precalculate factorials */
    idivide_f32(&x2,  &x2,  2      );
    idivide_f32(&x4,  &x4,  24     );
    idivide_f32(&x6,  &x6,  720    );
    idivide_f32(&x8,  &x8,  40320  );
    idivide_f32(&x10, &x10, 3628800);

    /* Add up the series */
    make_f32(ret, 1, 0);
    subtract_f32(ret, ret, &x2 );
    add_f32     (ret, ret, &x4 );
    subtract_f32(ret, ret, &x6 );
    add_f32     (ret, ret, &x8 );
    subtract_f32(ret, ret, &x10);
    
    /* Trim off any negative answers, since this is due to inaccuracy */
    if (is_negative_f32(ret))
    {
        *ret = plus_zero;
    }
}

/*
 * Argument should be positive (sign of argument is ignored).
 * Code adapted from the C Snippets Archive (public domain).
 */
void square_root_f32(f32_t *ret, const f32_t *a)
{
    uint32_t x   = a->mantissa;
    int exponent = a->exponent;
    uint32_t acc = 0L;          /* accumulator   */
    uint32_t rem = 0L;          /* remainder     */
    uint32_t est = 0L;          /* trial product */
    int i;

    if (a->exponent & 1)
    {
        x >>= 1;
        exponent += 1;
    }
    for (i = 0; i < 32; ++i)
    {
        rem = (rem << 2) + ((x & 0xc0000000u) >> 30);
        x <<= 2;
        acc <<= 1;
        est = (acc << 1) + 1;
        if (rem >= est)
        {
            rem -= est;
            ++acc;
        }
    }
    ret->mantissa = acc;
    ret->exponent = (exponent >> 1) - 16;
    normalise_f32(ret);
}

void abs_f32(f32_t *ret, const f32_t *a)
{
    ret->mantissa = a->mantissa;
    ret->exponent = a->exponent;
    ret->signum   = 1;
}

bool is_ge_f32(const f32_t *a, const f32_t *b)
{
    if (a->signum > 0)
    {
        if (b->signum > 0)          /* both positive */
        {
            if (a->exponent > b->exponent)
            {
                return true;
            }
            if (a->exponent < b->exponent)
            {
                return false;
            }
            return (a->mantissa >= b->mantissa);
        }
        else                        /* a postive, b negative */
        {
            return true;
        }
    }
    else
    {
        if (b->signum > 0)          /* a negative, b positive */
        {
            return false;
        }
        else                        /* both negative */
        {
            if (a->exponent < b->exponent)
            {
                return true;
            }
            if (a->exponent > b->exponent)
            {
                return false;
            }
            return (a->mantissa <= b->mantissa);
        }
    }
}

#if INCLUDE_F32_TESTS
#include "util.h"
#include "m0rtos.h"

struct ff_s
{
    float x;
    float y;
    float x_times_y;
    float x_over_y;
    float y_over_x;
    float x_plus_y;
    float x_minus_y;
    float y_minus_x;
    bool  x_ge_y;
    bool  y_ge_x;
};

struct fi_s
{
    float x;
    int32_t y;
    float x_times_y;
    float x_over_y;
    float x_plus_y;
    float x_minus_y;
};

struct f_s
{
    float x;
    float answer;
};

static const struct ff_s test_ff[] = 
{
    /*  x           y             x*y       x/y            y/x           x+y         x-y           y-x          x>=y    y>=x */
    { 1.23456e-6, 1e10,        12345.6,     1.23456e-16,  8.10005184e15, 1e10,       -1e10,        1e10,       false, true },
    { 3.33333333, 1.11111111,  3.70370369,  3.0,          0.33333333,    4.44444444,  2.22222222, -2.22222222, true,  false},
    { 3.33333333, 3.33333333,  11.1111111,  1.0,          1.0,           6.66666666,  0.0,         0.0,        true,  true },
    {-3.33333333, 1.11111111, -3.70370369, -3.0,         -0.33333333,   -2.22222222, -4.44444444,  4.44444444, false, true },
    {-3.33333333, 3.33333333, -11.1111111, -1.0,         -1.0,           0.0,        -6.66666666,  6.66666666, false, true },
};
static const struct fi_s test_fi[] =
{
    /*   x          y         x*y            x/y             x+y            x-y    */
    {1.23456e+6, 1000000000, 1.23456e15,    1.23456e-3,     1001234560,   -998765440   },
    {3.33333333, 1000000000, 3.33333333e+9, 3.33333333e-9,  1000000003.3, -1000000003.3},
};
static const struct f_s test_cosine[] = 
{
    /* x         cosine(x) */
    { 0.0,         1.0},
    {-0.0,         1.0},
    { 1.5707963,   0.0},
    {-1.5707963,   0.0},
    { 0.785398163, 0.70710678},
    {-0.785398163, 0.70710678},
    {0.00001,      0.9999999999},
};

static const struct f_s test_square_root[] = 
{
    /* x           square_root(x) */
    { 25600.0,     160.0        },
    {9.876543e+15, 99380797.9   },
    {9.876543e-15, 9.93807979e-8},
};

static const f32_t max_error = {0x80000000, -54, 1};

static bool close_enough(const f32_t *z, const f32_t *a)
{
    f32_t error;
    
    if (a->mantissa != 0)
    {
        divide_f32(&error, z, a);       /* Proportional error */
        iadd_f32(&error, &error, -1);
    }
    else
    {
        error = *z;                     /* Absolute error if answer is zero */
    }
    abs_f32(&error, &error);
    
    return is_ge_f32(&max_error, &error);
}

static void check_answer_fff(f32_t *x, f32_t *y, f32_t *z, f32_t *a, const char *op)
{
    bool pass;
    
    pass = close_enough(z, a);
    
    dprintf("%s %09f %s %09f = %09f, should be %09f\n", pass ? " PASS" : "*FAIL", x, op, y, z, a);
    sleep(10);
}

static void check_answer_ffb(f32_t *x, f32_t *y, bool z, bool a, const char *op)
{
    bool pass;
    
    pass = z == a;
    
    dprintf("%s %09f %s %09f = %s, should be %s\n", pass ? " PASS" : "*FAIL", x, op, y,
            z ? "T" : "F", a ? "T" : "F");
    sleep(10);
}

static void check_answer_fif(f32_t *x, int32_t iy, f32_t *z, f32_t *a, const char *op)
{
    bool pass;
    
    pass = close_enough(z, a);
    
    dprintf("%s %09f %s %d = %09f, should be %09f\n", pass ? " PASS" : "*FAIL", x, op, iy, z, a);
    sleep(10);
}

static void check_answer_ff(f32_t *x, f32_t *z, f32_t *a, const char *op)
{
    bool pass;
    
    pass = close_enough(z, a);
    
    dprintf("%s %s %09f = %09f, should be %09f\n", pass ? " PASS" : "*FAIL", op, x, z, a);
    sleep(10);
}

void f32_test(void)
{
    int i;
    f32_t x, y, z, a;
    int32_t iy;
    uint32_t ticks1, ticks2;

    for (i = 0; i < sizeof(test_ff) / sizeof(test_ff[0]); ++i)
    {
        get_f32_from_float(&x, test_ff[i].x);
        get_f32_from_float(&y, test_ff[i].y);

        multiply_f32(&z, &x, &y);
        get_f32_from_float(&a, test_ff[i].x_times_y);
        check_answer_fff(&x, &y, &z, &a, "x");
        multiply_f32(&z, &y, &x);
        check_answer_fff(&y, &x, &z, &a, "x");

        divide_f32(&z, &x, &y);
        get_f32_from_float(&a, test_ff[i].x_over_y);
        check_answer_fff(&x, &y, &z, &a, "/");
        divide_f32(&z, &y, &x);
        get_f32_from_float(&a, test_ff[i].y_over_x);
        check_answer_fff(&y, &x, &z, &a, "/");

        add_f32(&z, &x, &y);
        get_f32_from_float(&a, test_ff[i].x_plus_y);
        check_answer_fff(&x, &y, &z, &a, "+");
        add_f32(&z, &y, &x);
        check_answer_fff(&y, &x, &z, &a, "+");

        subtract_f32(&z, &x, &y);
        get_f32_from_float(&a, test_ff[i].x_minus_y);
        check_answer_fff(&x, &y, &z, &a, "-");
        subtract_f32(&z, &y, &x);
        get_f32_from_float(&a, test_ff[i].y_minus_x);
        check_answer_fff(&y, &x, &z, &a, "-");
        
        check_answer_ffb(&x, &y, is_ge_f32(&x, &y), test_ff[i].x_ge_y, ">=");
        check_answer_ffb(&y, &x, is_ge_f32(&y, &x), test_ff[i].y_ge_x, ">=");
    }

    for (i = 0; i < sizeof(test_fi) / sizeof(test_fi[0]); ++i)
    {
        get_f32_from_float(&x, test_fi[i].x);
        iy = test_fi[i].y;

        imultiply_f32(&z, &x, iy);
        get_f32_from_float(&a, test_fi[i].x_times_y);
        check_answer_fif(&x, iy, &z, &a, "x");

        idivide_f32(&z, &x, iy);
        get_f32_from_float(&a, test_fi[i].x_over_y);
        check_answer_fif(&x, iy, &z, &a, "/");

        iadd_f32(&z, &x, iy);
        get_f32_from_float(&a, test_fi[i].x_plus_y);
        check_answer_fif(&x, iy, &z, &a, "+");

        iadd_f32(&z, &x, -iy);
        get_f32_from_float(&a, test_fi[i].x_minus_y);
        check_answer_fif(&x, iy, &z, &a, "-");
    }
    
    for (i = 0; i < sizeof(test_cosine) / sizeof(test_cosine[0]); ++i)
    {
        get_f32_from_float(&x, test_cosine[i].x);
        cosine_f32(&z, &x);
        get_f32_from_float(&a, test_cosine[i].answer);
        check_answer_ff(&x, &z, &a, "cosine");
    }

    for (i = 0; i < sizeof(test_square_root) / sizeof(test_square_root[0]); ++i)
    {
        get_f32_from_float(&x, test_square_root[i].x);
        square_root_f32(&z, &x);
        get_f32_from_float(&a, test_square_root[i].answer);
        check_answer_ff(&x, &z, &a, "square_root");
    }

    
    ticks1 = ticks;
    while (ticks1 == ticks);
    ticks1 = ticks;
    for (i = 0; i < 32000; ++i)
    {
        add_f32(&z, &x, &y);
    }
    ticks2 = ticks;
    dprintf("Add took %u ticks\n", ticks2 - ticks1);
    
    ticks1 = ticks;
    while (ticks1 == ticks);
    ticks1 = ticks;
    for (i = 0; i < 32000; ++i)
    {
        multiply_f32(&z, &x, &y);
    }
    ticks2 = ticks;
    dprintf("Multiply took %u ticks\n", ticks2 - ticks1);
    
    ticks1 = ticks;
    while (ticks1 == ticks);
    ticks1 = ticks;
    for (i = 0; i < 32000; ++i)
    {
        divide_f32(&z, &x, &y);
    }
    ticks2 = ticks;
    dprintf("Divide took %u ticks\n", ticks2 - ticks1);

    ticks1 = ticks;
    while (ticks1 == ticks);
    ticks1 = ticks;
    for (i = 0; i < 32000; ++i)
    {
        square_root_f32(&z, &x);
    }
    ticks2 = ticks;
    dprintf("Square root took %u ticks\n", ticks2 - ticks1);

}
#endif /* INCLUDE_F32_TESTS */
