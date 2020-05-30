/*
 * float32.h
 *
 *  Created on: May 29, 2020
 *      Author: Jcallan
 *
 *  Routines for handling floating point numbers with 32 bits
 * of mantissa, 8 bits of exponent and a sign bit
 */

#ifndef FLOAT32_H_
#define FLOAT32_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * A floating-point number is stored as an unsigned 32-bit mantissa
 * and an exponent value (-128 to +127).
 * So  (0x120000000, -17,  1) represents the number 2304.0
 * and (0xff800000,   24, -1) represents the number -71916856549572608.0
 * and (0x3456789a,  -70,  1) represents the number 7.437645554e-13
 */
typedef struct
{
    uint32_t mantissa;
    int8_t   exponent;
    int8_t   signum;
    int16_t  pad;
} f32_t;

extern void get_f32_from_float(f32_t *a, float f);
extern int normalise_f32(f32_t *a);

/* 
 * This routine prints out a floating-point number in decimal.
 */
extern int sprint_f32(char *buffer, const f32_t *a, int decimal_places, bool plus, bool pad_zeroes);

/*
 * This routine scans a string for a floating point value, and writes it into a f32_t
 * It returns the number of characters consumed if successful, otherwise negative.
 * At most 9 decimal places are used.
 * Number must be of the form [-]nnnn.nnnnnnnn
 *   ie optional whitespace and sign, at least one digit, decimal point, at least one digit
 */
extern int parse_f32(f32_t *ret, const char *buffer, int p_answer);

/*
 * These routines accomplish 
 *    ret = a o b
 * where a and b are floating-point numbers. 
 */
extern void multiply_f32(f32_t *ret, const f32_t *a, const f32_t *b);
extern void   divide_f32(f32_t *ret, const f32_t *a, const f32_t *b);
extern void      add_f32(f32_t *ret, const f32_t *a, const f32_t *b);
extern void subtract_f32(f32_t *ret, const f32_t *a, const f32_t *b);

/* 
 * These routines are the same, but the 'b' argument is an int32_t instead of a
 * pointer to a floating-point number.
 */
extern void imultiply_f32(f32_t *ret, const f32_t *a, int32_t b);
extern void   idivide_f32(f32_t *ret, const f32_t *a, int32_t b);
extern void      iadd_f32(f32_t *ret, const f32_t *a, int32_t b);

/*
 * Trigonometric routines
 * Angle must be between -pi/2 and pi/2
 */
extern void cosine_f32(f32_t *ret, const f32_t *a);

/* Utility functions */

/*
 * Return the square root of a floating point number
 */
extern void square_root_f32(f32_t *ret, const f32_t *a);

/*
 * Return the absolute value of a floating-point number
 */
extern void abs_f32(f32_t *ret, const f32_t *a);

/*
 * Create a floatingpoint number, specifying the mantissa and exponent
 */
static __inline void make_f32(f32_t *a, int32_t mantissa, int exponent)
{
    if (mantissa >= 0)
    {
        a->mantissa = mantissa;
        a->signum   = 1;
    }
    else
    {
        a->mantissa = mantissa;
        a->signum   = -1;
    }
    a->exponent = exponent;
    
    normalise_f32(a);
}

/*
 * Convert a floating point number to an integer, truncating at the binary point
 */
extern int32_t get_int32(const f32_t *a);

static __inline int is_negative_f32(const f32_t *a)
{
    return (a->signum < 0);
}

extern void f32_test(void);

extern const f32_t pi, half_pi, quarter_pi, two_pi, third_pi, two_thirds_pi, one_sixth_pi;

#endif /* FLOAT32_H_ */
