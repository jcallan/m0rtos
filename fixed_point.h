/*
 * fixed_point.h
 *
 *  Created on: Apr 25, 2014
 *      Author: Jcallan
 *
 *  Routines for handling fixed point numbers
 */

#ifndef FIXED_POINT_H_
#define FIXED_POINT_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * A fixed-point number is stored as a signed 32-bit mantissa
 * and a 'precision' value which records the number of bits after the binary point (0-31).
 * So  (0x00008000, 12) represents the number 8.0
 * and (0xff800000, 24) represents the number -0.5
 */
typedef struct
{
    int32_t mantissa;
    int     precision;
} fix32_t;

#define FIX32_FROM_FLOAT(f, precision) {(int32_t)((f) * (1ul << (precision))), (precision)}
/* 
 * This routine prints out a fixed-point number in decimal.
 */
extern int sprint_fix32(char *buffer, const fix32_t *a, int decimal_places, bool plus,
                        bool pad_zeroes);

/*
 * This routine scans a string for a floating point value, and writes it into a fix32_t
 * It returns the number of characters consumed if successful, otherwise negative.
 * At most 9 decimal places are used.
 * Number must be of the form [-]nnnn.nnnnnnnn
 *   ie optional whitespace and sign, at least one digit, decimal point, at least one digit
 */
extern int parse_fix32(fix32_t *ret, const char *buffer, int p_answer);

/*
 * These routines accomplish 
 *    ret = a o b
 * where a and b are fixed point numbers. The precision of the returned number is
 * specified in the precision argument. Specify precision as -1 to use automatic precision.
 */
extern void multiply_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int precision);
extern void   divide_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b);
extern void      add_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int precision);
extern void subtract_fix32(fix32_t *ret, const fix32_t *a, const fix32_t *b, int precision);

/* 
 * These routines are the same, but the 'b' argument is an int32_t instead of a
 * pointer to a fixed-point number.
 */
extern void imultiply_fix32(fix32_t *ret, const fix32_t *a, int32_t b, int precision);
extern void   idivide_fix32(fix32_t *ret, const fix32_t *a, int32_t b);
extern void      iadd_fix32(fix32_t *ret, const fix32_t *a, int32_t b, int precision);

/*
 * Trigonometric routines
 * Angle must be between -pi/2 and pi/2
 */
extern void cosine_fix32(fix32_t *ret, const fix32_t *a);

/* Utility functions */

/*
 * Return the square root of a fixed point number
 */
extern void square_root_fix32(fix32_t *ret, const fix32_t *a);

/*
 * Return the absolute value of a fixed point number
 */
extern void abs_fix32(fix32_t *ret, const fix32_t *a);


/*
 * Set precision to the maximum possible
 * Useful when using automatic precision in the routines above
 */
extern void normalise_fix32(fix32_t *a);

/*
 * Create a fixed point number, specifying the mantissa and the precision
 */
static __inline void make_fix32(fix32_t *a, int32_t mantissa, int precision)
{
    a->mantissa  = mantissa;
    a->precision = precision;
}

/*
 * Convert a fixed point number to an integer, truncating at the binary point
 */
static __inline int32_t get_int32(const fix32_t *a)
{
    int32_t ret;

    ret = a->mantissa >> a->precision;

    return ret;
}

static __inline int is_negative_fix32(const fix32_t *a)
{
    return (a->mantissa < 0);
}

extern const fix32_t pi, half_pi, quarter_pi, two_pi, third_pi, two_thirds_pi, one_sixth_pi;

#endif /* FIXED_POINT_H_ */
