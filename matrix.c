#include <math.h>
#include "impulso.h"

/* Macro to get a[i][j] where a has cols columns: can be used as an lvalue */
#define element(a, i, j, cols) (a[(i) * (cols) + (j)])

/*
 * Multiply an n x m matrix by an m x n matrix
 * z = x * y
 * z is an n x n matrix
 */
void matrix_multiply(float *z, const float *x, const float *y, int n, int m)
{
    int i, j, k;
    float f_tmp;
    
    for (i = 0; i < n; ++i)
    {
        for (j = 0; j < n; ++j)
        {
            f_tmp = 0.0f;
            for (k = 0; k < m; ++k)
            {
                f_tmp += element(x, i, k, m) * element(y, k, j, n);
            }
            element(z, i, j, n) = f_tmp;
        }
    }
}

/*
 * Perform Gaussian elimination on a matrix with m rows and n columns
 * Operates in place
 */
void matrix_g_elim(float *a, unsigned m, unsigned n)
{
    int i, j, i_max, h, k;
    float f_max, f_tmp;

    h = 0;      /* Pivot row */
    k = 0;      /* Pivot column */
    while ((h < m) && (k < n))
    {
        /* Find the k-th pivot: maximum absolute value in column k */
        f_max = 0.0f;
        i_max = 0;
        for (i = h; i < m; ++i)
        {
            f_tmp = fabsf(element(a, i, k, n));
            if (f_tmp > f_max)
            {
                f_max = f_tmp;
                i_max = i;
            }
        }
        
        /* Did we find a pivot? */
        if (element(a, i_max, k, n) == 0.0f)
        {
            /* No pivot in this column, pass to next column */
            ++k;
        }
        else
        {
            /* Swap rows h and i_max */
            for (j = 0; j < n; ++j)
            {
                f_tmp = element(a, h, j, n);
                element(a, h, j, n) = element(a, i_max, j, n);
                element(a, i_max, j, n) = f_tmp;
            }
            /* Do for all rows below pivot: */
            for (i = h + 1; i < m; ++i)
             {
                 f_tmp = element(a, i, k, n) / element(a, h, k, n);
                 /* Fill with zeros the lower part of pivot column: */
                 element(a, i, k, n) = 0.0f;
                 /* Do for all remaining elements in current row: */
                 for (j = k + 1; j < n; ++j)
                 {
                     element(a, i, j, n) = element(a, i, j, n) - element(a, h, j, n) * f_tmp;
                 }
             }
             /* Increase pivot row and column */
             ++h;
             ++k;
         }
    }
}

/*
 * Takes an m x n matrix in row-echelon form and reduces the left side to the identity matrix Im
 * Operates in place
 */
int matrix_back_subs(float *a, unsigned m, unsigned n)
{
    int i, j, k;
    float f_tmp;
    
    for (i = m - 1; i >= 0; --i)
    {
        f_tmp = element(a, i, i, n);
        if (f_tmp == 0.0f)
        {
            return -1;          /* No solution, matrix not invertible */
        }
        /* Divide this row so that the leading element is 1 */
        for (j = i; j < n; ++j)
        {
            element(a, i, j, n) /= f_tmp;
        }
        /* Subtract a multiple of this row from each row above, to create a column of zeroes */
        for (k = i - 1; k >= 0; --k)
        {
            f_tmp = element(a, k, i, n);
            element(a, k, i, n) = 0.0f;
            for (j = i + 1; j < n; ++j)
            {
                element(a, k, j, n) -= element(a, i, j, n) * f_tmp;
            }
        }
    }

    return 0;
}

/*
 * Print out a matrix
 */
void matrix_print(const float *x, unsigned rows, unsigned cols)
{
    int i, j;

    for (i = 0; i < rows; ++i)
    {
        for (j = 0; j < cols; ++j)
        {
            dprintf("%+03.4f ", element(x, i, j, cols));
        }
        dprintf("\n");
        flush_serial_tx();
    }
}

/*
 * Get the transpose of an m x n matrix
 * y = xT
 */
void matrix_transpose(float *y, const float *x, int m, int n)
{
    int i, j;
    
    for (i = 0; i < m; ++i)
    {
        for (j = 0; j < n; ++j)
        {
            element(y, j, i, m) = element(x, i, j, n);
        }
    }
}

/*
 * Invert an n x n matrix
 * Requires a workspace of (n * 2n) floats
 * y and x can be the same pointer
 */
int matrix_invert(float *y, const float *x, int n, float *ws)
{
    int rows = n;
    int cols = n * 2;
    int i, j, ret;

    /* Copy x into our workspace, and write the identity matrix to the right */
    for (i = 0; i < rows; ++i)
    {
        /* Copy x */
        for (j = 0; j < rows; ++j)
        {
            element(ws, i, j, cols) = element(x, i, j, rows);
        }
        /* Add identity */
        for (j = rows; j < cols; ++j)
        {
            element(ws, i, j, cols) = (i + rows == j ? 1.0f : 0.0f);
        }
    }

    //dprintf("\nWorkspace is...\n");
    //matrix_print(ws, rows, cols);
    
    matrix_g_elim(ws, rows, cols);

    //dprintf("After elimination, workspace is...\n");
    //matrix_print(ws, rows, cols);
    
    ret = matrix_back_subs(ws, rows, cols);
    if (ret < 0)
    {
        return ret;
    }
    
    //dprintf("After back subtitution, workspace is...\n");
    //matrix_print(ws, rows, cols);
    
    /* Copy right side of workspace to y */
    for (i = 0; i < rows; ++i)
    {
        for (j = 0; j < rows; ++j)
        {
            element(y, i, j, rows) = element(ws, i, j + rows, cols);
        }
    }
    
    return 0;
}

float x[7 * 14];
float y[7 * 7];
float q[4 * 3] = {1,  9, -1,
                  0,  0,  5,
                  1, -1,  0,
                  0,  3,  2};

const float z[49] = {-11.5,3.1,7.1,-4.1,5.1,6.1,7.1,
                     9.5,0,0,1,-14,0,14,
                     0.85, -1.6,1,-1,0,-2, -2,
                     0.5,-2.3,0,1.4,1.5,1.5,0.14,
                     20.5,1.9,1.8,1.7,1.6,1.5,1.4,
                     36.5,0,38,0,40,0,4.2,
                     10.5,1,1,1,1,1,1};

void matrix_test(void)
{
    dprintf("\nq is:\n");
    matrix_print(q, 4, 3);
    
    matrix_transpose(y, q, 4, 3);
    dprintf("\nTranspose is:\n");
    matrix_print(y, 3, 4);

    dprintf("\nz is:\n");
    matrix_print(z, 7, 7);

    matrix_invert(y, z, 7, x);
    dprintf("\nInverse is:\n");
    matrix_print(y, 7, 7);

    matrix_multiply(q, y, z, 7, 7);
    dprintf("\nProduct is:\n");
    matrix_print(q, 7, 7);
}
