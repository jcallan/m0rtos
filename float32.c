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

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

const f32_t pi            = {1686629713UL, -29, +1};     /* TODO: one more bit of precision possible */
const f32_t half_pi       = {1686629713UL, -30, +1};
const f32_t quarter_pi    = {1686629713UL, -31, +1};
const f32_t two_pi        = {1686629713UL, -28, +1};
const f32_t third_pi      = {1124419809UL, -30, +1};
const f32_t two_thirds_pi = {1124419809UL, -29, +1};
const f32_t one_sixth_pi  = {1124419809UL, -31, +1};

int _print_integer(char *buffer, uint32_t integer)
{
    uint32_t place_val, digit;
    int len = 0;
    bool started = false;
    
    for (place_val = 1000000000; place_val >= 1; place_val /= 10)
    {
        digit = integer / place_val;
        if (started || (digit > 0) || (place_val == 1))
        {
            integer -= digit * place_val;
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
    int i;
    int len = 0;
                                       
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
    int len = 0;
    uint32_t integer, mantissa;
    uint32_t fraction;
    bool scientific;

    if (a->signum < 0)
    {
        buffer[len] = '-';
        ++len;
    }
    else
    {
        if (plus)
        {
            buffer[len] = '+';
            ++len;
        }
    }
    
    if (a->exponent > 0 || a->exponent < -64)
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

    len += _print_integer(buffer, integer);
    
    if (decimal_places > 0)
    {
        buffer[len] = '.';
        ++len;
        len += _print_fraction(buffer + len, fraction, decimal_places, zeroes);
    }

    if (scientific)
    {
        buffer[len] = 'b';
        ++len;
        len += _print_integer(buffer + len, a->exponent + 31);
    }
    
    buffer[len] = 0;
    return len;
}

#if 0
int parse_fix32(fix32_t *ret, const char *buffer, int p_answer)
{
    int rc = -1, fraction_length = 0, consumed;
    const char *decimal_ptr = NULL;
    int32_t integer_part, fraction_part = 0;
    uint32_t fraction_divider = 1;
    
    consumed = get_integer(buffer, &integer_part);
    if ((consumed > 0) && (buffer[consumed] == '.'))
    {
        ++consumed;
        decimal_ptr = &buffer[consumed];
        consumed += get_integer(decimal_ptr, &fraction_part);
    }

    if (ret > 0)
    {
        if (decimal_ptr)
        {
            /* Find length of fraction part */
            while ((decimal_ptr[fraction_length] >= '0') && (decimal_ptr[fraction_length] <= '9'))
            {
                fraction_divider *= 10;
                ++fraction_length;
                if (fraction_length >= 9)
                {
                    break;
                }
            }
        }

        /* Add the integer and fraction parts together */
        if (integer_part < 0)
        {
            fraction_part = -fraction_part;
        }
        make_fix32(ret, fraction_part, 0);
        idivide_fix32(ret, ret, fraction_divider, 31);
        iadd_fix32(ret, ret, integer_part, p_answer);
        rc = consumed;
    }

    return rc;
}
#endif

/*
 * Turn a float into an f32_t
 */
void get_f32_from_float(f32_t *a, float f)
{
    uint32_t val;

    /* Read the 32 bits of the float into val */
    val = *(uint32_t *)&f;
    
    /* Now pick out the mantissa, exponent and sign */
    a->mantissa = ((val & 0x007fffffu) <<  8) | 0x80000000u;
    a->exponent = ((val & 0x7f800000u) >> 23) - 158;
    a->signum   =  (val & 0x80000000u) ? -1 : +1;
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
    
    shift = count_leading_space(a->mantissa);
    new_exponent = a->exponent - shift;
    if (new_exponent < INT8_MIN)
    {
        shift -= new_exponent - INT8_MIN;
    }
    a->mantissa <<= shift;
    a->exponent -= shift;
}

/*
 * The functions multiply_f32, divide_f32, add_f32 and subtract_f32
 * rely on shifting 64-bit signed types. From the armcc documentation:
 * 
 * Right shifts on signed quantities are arithmetic.
 * For values of type long long, shifts outside the range 0 to 63 are undefined.
 *
 * So we don't need to worry about negative mantissas, but we do need to correct
 * for the cases where the shift amount could be negative or larger than 63.
 */

void multiply_f32(f32_t *ret, const f32_t *a, const f32_t *b)
{
    uint32_t mantissa;
    int exponent, signum;

    mantissa = ((uint64_t)a->mantissa * b->mantissa) >> 32;
    exponent = a->exponent + b->exponent + 32;
    signum   = (a->signum == b->signum ? 1 : -1);
    if (exponent > INT8_MAX)
    {
        exponent = INT8_MAX;
        mantissa = UINT32_MAX;
    }
    if (exponent < INT8_MIN)
    {
        exponent = 0;
        mantissa = 0;
    }

    ret->mantissa = mantissa;
    ret->exponent = exponent;
    ret->signum   = signum;
    normalise_f32(ret);
}

void imultiply_fix32(f32_t *ret, const f32_t *a, int32_t b, int p_answer)
{
    f32_t bb;

    bb.mantissa = b >= 0 ? b : -b;
    bb.signum   = b >= 0 ? 1 : -1;
    bb.exponent = 0;

    multiply_f32(ret, a, &bb);
}

#if 0
void divide_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b)
{
    uint32_t numerator, denominator, answer, current_bit;
    bool negative;
    int precision, i, gap;
    
    /* Make both operands positive */
    if (a->mantissa < 0)
    {
        numerator = -a->mantissa;
        negative = true;
    }
    else
    {
        numerator = a->mantissa;
        negative = false;
    }
    if (b->mantissa < 0)
    {
        denominator = -b->mantissa;
        negative = !negative;
    }
    else
    {
        denominator = b->mantissa;
    }

    /* Check for zeroes in either numerator or denominator */
    if (denominator == 0 || numerator == 0)
    {
        ret->mantissa  = 0;
        ret->precision = 0;
        return;
    }
    
    /* Note precision, and shift both operands so they are at the top of the range */
    precision = 30 + a->precision - b->precision;
    if (numerator < (1u << 16))
    {
        numerator <<= 16;
        precision += 16;
    }
    if (denominator < (1u << 16))
    {
        denominator <<= 16;
        precision -= 16;
    }
    if (numerator < (1u << 24))
    {
        numerator <<= 8;
        precision += 8;
    }
    if (denominator < (1u << 24))
    {
        denominator <<= 8;
        precision -= 8;
    }
    if (numerator < (1u << 28))
    {
        numerator <<= 4;
        precision += 4;
    }
    if (denominator < (1u << 28))
    {
        denominator <<= 4;
        precision -= 4;
    }
    if (numerator < (1u << 30))
    {
        numerator <<= 2;
        precision += 2;
    }
    if (denominator < (1u << 30))
    {
        denominator <<= 2;
        precision -= 2;
    }
    if (numerator < (1u << 31))
    {
        numerator <<= 1;
        precision += 1;
    }
    if (denominator < (1u << 31))
    {
        denominator <<= 1;
        precision -= 1;
    }
    /* We want the denominator as big as possible, but not bigger than the numerator */
    if (denominator > numerator)
    {
        denominator >>= 1;
        ++precision;
    }
    
    /* Check for overflow */
    if (precision < 0)
    {
        ret->mantissa  = (negative ? INT32_MIN : INT32_MAX);
        ret->precision = 0;
        return;
    }
    /* Check for underflow */
    if (precision > 31)
    {
        gap = precision - 31;
        if (gap > 31)
        {
            ret->mantissa  = 0;
            ret->precision = 0;
            return;
        }
        precision -= gap;
    }
    else
    {
        gap = 0;
    }
    
    /*
     * Do the long division
     *
     * This can be made slightly more accurate by using 64 bit values, e.g:
     *     uint64_t numerator64   = ((uint64_t)numerator)   << 32;
     *     uint64_t denominator64 = ((uint64_t)denominator) << 32;
     * But this costs quite a lot more cycles, perhaps 900 vs 450.
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
    
    /* Fix up the answer - shift down by at least one to make room for the sign bit */
    answer >>= gap + 1;
    if (negative)
    {
        ret->mantissa = -(int32_t)answer;
    }
    else
    {
        ret->mantissa = (int32_t)answer;
    }
    ret->precision = precision;
}

void idivide_fix32(fix32_t *ret, const fix32_t *a, int32_t b)
{
    fix32_t bb;

    bb.mantissa = b;
    bb.precision = 0;

    divide_fix32(ret, a, &bb);
}

void add_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int p_answer)
{
    int64_t a64, b64;
    
    /* Automatic precision? */
    if (p_answer < 0)
    {
        p_answer = MIN(a->precision, b->precision);
        if (p_answer > 0)
        {
            --p_answer;
        }
    }
    
    /* Convert the numbers to 32.32 before adding them */
    a64 = a->mantissa;
    a64 <<= 32 - a->precision;                /* Shift range is +1 to +32 */

    b64 = b->mantissa;
    b64 <<= 32 - b->precision;                /* Shift range is +1 to +32 */

    a64 += b64;
    a64 >>= 32 - p_answer;                    /* Shift range is +1 to +32 */

    ret->mantissa  = (int32_t)a64;
    ret->precision = p_answer;
}

void subtract_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int p_answer)
{
    int64_t a64, b64;
    
    /* Automatic precision? */
    if (p_answer < 0)
    {
        p_answer = MIN(a->precision, b->precision);
        if (p_answer > 0)
        {
            --p_answer;
        }
    }
    
    /* Convert the numbers to 32.32 before subtracting them */
    a64 = a->mantissa;
    a64 <<= 32 - a->precision;                /* Shift range is +1 to +32 */

    b64 = b->mantissa;
    b64 <<= 32 - b->precision;                /* Shift range is +1 to +32 */

    a64 -= b64;
    a64 >>= 32 - p_answer;                    /* Shift range is +1 to +32 */

    ret->mantissa  = (int32_t)a64;
    ret->precision = p_answer;
}

void iadd_fix32(fix32_t *ret, const fix32_t *a, int32_t b, int p_answer)
{
    fix32_t bb;

    bb.mantissa = b;
    bb.precision = 0;

    add_fix32(ret, a, &bb, p_answer);
}

/* 
 * Uses Taylor expansion: cosine(x) = 1 - x^2/2! + x^4/4! - x^6/6! + ...
 * We assume that -pi/2 <= a <= pi/2
 */
void cosine_fix32(fix32_t *ret, const fix32_t *a)
{
    fix32_t x2, x4, x6, x8, x10;

    /* Calculate the powers of x we need - will always be non-negative */
    multiply_fix32(&x2,  a,   a,   29);    /* Largest case  2, hence 29 bits of precision */
    multiply_fix32(&x4,  &x2, &x2, 28);    /* Largest case  6, hence 28 bits of precision */
    multiply_fix32(&x6,  &x4, &x2, 27);    /* Largest case 15, hence 27 bits of precision */
    multiply_fix32(&x8,  &x4, &x4, 25);    /* Largest case 37, hence 25 bits of precision */
    multiply_fix32(&x10, &x4, &x6, 24);    /* Largest case 91, hence 24 bits of precision */

    /* Divide each term by the relevant factorial */
    idivide_fix32(&x2,  &x2,  2      );
    idivide_fix32(&x4,  &x4,  24     );
    idivide_fix32(&x6,  &x6,  720    );
    idivide_fix32(&x8,  &x8,  40320  );
    idivide_fix32(&x10, &x10, 3628800);

    /* Add up the series */
    make_fix32(ret, 1, 0);
    subtract_fix32(ret, ret, &x2,  30);
    add_fix32     (ret, ret, &x4,  30);
    subtract_fix32(ret, ret, &x6,  30);
    add_fix32     (ret, ret, &x8,  30);
    subtract_fix32(ret, ret, &x10, 30);
    
    /* Trim off any negative answers, since this is due to inaccuracy */
    if (is_negative_fix32(ret))
    {
        ret->mantissa = 0L;
    }
}

/*
 * Mantissa must be positive!
 * Code adapted from the C Snippets Archive (public domain).
 */
void square_root_fix32(fix32_t *ret, const fix32_t *a)
{
    uint32_t x = a->mantissa;
    int precision = a->precision;
    uint32_t acc = 0L;          /* accumulator   */
    uint32_t rem = 0L;          /* remainder     */
    uint32_t est = 0L;          /* trial product */
    int i;

    if (a->precision & 1)
    {
        x >>= 1;
        precision -= 1;
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
    if (acc > INT32_MAX)
    {
        acc >>= 1;
        precision -= 2;     /* subtract 2 because it is about to be halved */
    }
    ret->mantissa = (int32_t)acc;
    ret->precision = (precision / 2) + 16;
}
#endif

void abs_f32(f32_t *ret, const f32_t *a)
{
    ret->mantissa = a->mantissa;
    ret->exponent = a->exponent;
    ret->signum   = 1;
}

#include "util.h"

void f32_test(void)
{
    
    f32_t x, y, z;

    get_f32_from_float(&x, 1.23456e-6);
    get_f32_from_float(&y, 1e10);
    
    multiply_f32(&z, &x, &y);
    
    dprintf("%9f x %9f = %9f\n", &x, &y, &z);
}
