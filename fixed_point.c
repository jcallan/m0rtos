/*
 * fixed_point.c
 *
 *  Created on: Apr 25, 2014
 *      Author: Jcallan
 *
 *  This file contains routines for handling fixed point numbers
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>         /* For strchr */
#include "fixed_point.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

const fix32_t pi            = {1686629713L, 29};
const fix32_t half_pi       = {1686629713L, 30};
const fix32_t quarter_pi    = {1686629713L, 31};
const fix32_t two_pi        = {1686629713L, 28};
const fix32_t third_pi      = {1124419809L, 30};
const fix32_t two_thirds_pi = {1124419809L, 29};
const fix32_t one_sixth_pi  = {1124419809L, 31};

int sprint_fix32(char *buffer, const fix32_t *a, int decimal_places, bool plus, bool zeroes)
{
    static const uint32_t point[10] = {0, 429496729u, 858993459u, 1288490188u, 1717986918u,
                                       2147483648u, 2576980377u, 3006477107u, 3435973836u,
                                       3865470566u};
    int len = 0, i, started = 0;
    int32_t  integer, mantissa;
    uint32_t fraction, place_val, digit, place;

    if (a->mantissa < 0)
    {
        if (a->mantissa == INT32_MIN)
        {
            /* Bit of a hack, but -INT32_MIN is INT32_MIN! */
            mantissa = INT32_MAX;
        }
        else
        {
            mantissa = -a->mantissa;
        }
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
        mantissa = a->mantissa;
    }
    integer  = mantissa >> a->precision;
    fraction = mantissa << (32 - a->precision);

    for (place_val = 1000000000; place_val >= 1; place_val /= 10)
    {
        digit = integer / place_val;
        if (started || (digit > 0) || (place_val == 1))
        {
            integer -= digit * place_val;
            buffer[len] = '0' + digit;
            ++len;
            started = 1;
        }
    }
    
    if (decimal_places > 0)
    {
        buffer[len] = '.';
        ++len;

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
 * Calculate by how much we could shift an int32_t without changing the sign bit
 */
static int count_leading_space(int32_t val)
{
    int space = 0;
    if (val == 0)
    {
        return 31;
    }
    if (val < 0)
    {
        val = -val;
    }
    if (val < (1u << 15))
    {
        val <<= 16;
        space += 16;
    }
    if (val < (1u << 23))
    {
        val <<= 8;
        space += 8;
    }
    if (val < (1u << 27))
    {
        val <<= 4;
        space += 4;
    }
    if (val < (1u << 29))
    {
        val <<= 2;
        space += 2;
    }
    if (val < (1u << 30))
    {
        space += 1;
    }
    
    return space;
}

void normalise_fix32(fix32_t *a)
{
    int shift;
    
    shift = count_leading_space(a->mantissa);
    if (a->precision + shift > 31)
    {
        shift = 31 - a->precision;
    }
    a->mantissa <<= shift;
    a->precision += shift;
}

/*
 * The functions multiply_fix32, divide_fix32, add_fix32 and subtract_fix32
 * rely on shifting 64-bit signed types. From the armcc documentation:
 * 
 * Right shifts on signed quantities are arithmetic.
 * For values of type long long, shifts outside the range 0 to 63 are undefined.
 *
 * So we don't need to worry about negative mantissas, but we do need to correct
 * for the cases where the shift amount could be negative or larger than 63.
 */

void multiply_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int p_answer)
{
    int64_t answer;
    int shift_down;

    /* Automatic precision? */
    if (p_answer < 0)
    {
        p_answer = a->precision + b->precision - 31;
        if (p_answer < 0)
        {
            p_answer = 0;
        }
        if (p_answer > 31)
        {
            p_answer = 31;
        }
    }

    shift_down = a->precision + b->precision - p_answer;    /* Range is -31 to +62 */
    
    answer = a->mantissa * (int64_t)b->mantissa;
    
    if (shift_down >= 0)
    {
        answer >>= shift_down;
    }
    else
    {
        answer <<= -shift_down;
    }

    ret->mantissa  = (int32_t)answer;
    ret->precision = p_answer;
}

void imultiply_fix32(fix32_t *ret, const fix32_t *a, int32_t b, int p_answer)
{
    fix32_t bb;

    bb.mantissa = b;
    bb.precision = 0;

    multiply_fix32(ret, a, &bb, p_answer);
}

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

void abs_fix32(fix32_t *ret, const fix32_t *a)
{
    if (is_negative_fix32(a))
    {
        ret->mantissa = -a->mantissa;
    }
    else
    {
        ret->mantissa = a->mantissa;
    }
    ret->precision = a->precision;
}
