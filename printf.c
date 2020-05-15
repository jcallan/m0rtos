/*
    Copyright 2001, 2002 Georges Menie (www.menie.org)
    stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
    putchar is the only external dependency for this file,
    if you have a working putchar, leave it commented out.
    If not, uncomment the define below and
    replace outbyte(c) by your own function call.
*/
#define putchar(c) outbyte(c)

extern void putchar(int c);

#include <stdarg.h>
#include <stdint.h>
#include "fixed_point.h"

static void printchar(char **str, int c)
{
    if (str) 
    {
        **str = c;
        ++(*str);
    }
    else
    {
        (void)putchar(c);
    }
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad)
{
    int pc = 0, padchar = ' ', len = 0;
    const char *ptr;

    if (width > 0) 
    {
        for (ptr = string; *ptr; ++ptr)
        {
            ++len;
        }
        if (len >= width)
        {
            width = 0;
        }
        else
        {
            width -= len;
        }
        if (pad & PAD_ZERO)
        {
            padchar = '0';
        }
    }
    if (!(pad & PAD_RIGHT)) 
    {
        for ( ; width > 0; --width) 
        {
            printchar(out, padchar);
            ++pc;
        }
    }
    for ( ; *string ; ++string) 
    {
        printchar(out, *string);
        ++pc;
    }
    for ( ; width > 0; --width)
    {
        printchar(out, padchar);
        ++pc;
    }

    return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    char *s;
    int t, neg = 0, pc = 0;
    unsigned int u = i;

    if (i == 0) 
    {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints (out, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0)
    {
        neg = 1;
        u = -i;
    }

    s = print_buf + PRINT_BUF_LEN - 1;
    *s = '\0';

    while (u) 
    {
        t = u % b;
        if( t >= 10 )
        {
            t += letbase - '0' - 10;
        }
        --s;
        *s = t + '0';
        u /= b;
    }

    if (neg)
    {
        if( width && (pad & PAD_ZERO) )
        {
            printchar(out, '-');
            ++pc;
            --width;
        }
        else 
        {
            --s;
            *s = '-';
        }
    }

    return pc + prints(out, s, width, pad);
}

static int printfloat( char **out, void *f, int width, bool plus, int pad)
{
    char buf[20];
    
    sprint_fix32(buf, f, width, plus, pad & PAD_ZERO);
    return prints(out, buf, 0, 0);
}

static int print(char **out, const char *format, va_list args )
{
    int width, pad;
    bool plus;
    int pc = 0;
    char scr[2];
    char *s;

    for (; *format != 0; ++format) 
    {
        if (*format == '%') 
        {
            ++format;
            width = pad = 0;
            plus = false;
            if (*format == '\0')
            {
                break;
            }
            if (*format == '%')
            {
                goto out;
            }
            if (*format == '+')
            {
                ++format;
                plus = true;
            }
            if (*format == '-')
            {
                ++format;
                pad = PAD_RIGHT;
            }
            while (*format == '0') 
            {
                ++format;
                pad |= PAD_ZERO;
            }
            for ( ; *format >= '0' && *format <= '9'; ++format) 
            {
                width *= 10;
                width += *format - '0';
            }
            if (*format == 'l') 
            {
                ++format;               /* We can safely ignore one l, long => 32 bits */
            }
            if( *format == 's' ) 
            {
                s = (char *)va_arg( args, int );
                pc += prints (out, s ? s : "(null)", width, pad);
                continue;
            }
            if( *format == 'd' )
            {
                pc += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a');
                continue;
            }
            if( *format == 'x' || *format == 'p' )
            {
                pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'X' )
            {
                pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A');
                continue;
            }
            if( *format == 'u' )
            {
                pc += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'c' )
            {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char)va_arg( args, int );
                scr[1] = '\0';
                pc += prints (out, scr, width, pad);
                continue;
            }
            if ( *format == 'f' )
            {
                pc += printfloat (out, va_arg( args, void * ), width, plus, pad);
            }
        }
        else 
        {
        out:
            printchar (out, *format);
            ++pc;
        }
    }
    if (out) **out = '\0';
    va_end( args );
    return pc;
}

int dprintf(const char *format, ...)
{
    va_list args;

    va_start( args, format );
    return print( 0, format, args );
}

int dsprintf(char *out, const char *format, ...)
{
    va_list args;

    va_start( args, format );
    return print( &out, format, args );
}

int dputs(const char *s)
{
    return dprintf("%s\n", s);
}

#ifdef TEST_PRINTF
int main(void)
{
    char *ptr = "Hello world!";
    char *np = 0;
    int i = 5;
    unsigned int bs = sizeof(int)*8;
    int mi;
    char buf[80];

    mi = (1 << (bs-1)) + 1;
    dprintf("%s\n", ptr);
    dprintf("printf test\n");
    dprintf("%s is null pointer\n", np);
    dprintf("%d = 5\n", i);
    dprintf("%d = - max int\n", mi);
    dprintf("char %c = 'a'\n", 'a');
    dprintf("hex %x = ff\n", 0xff);
    dprintf("hex %02x = 00\n", 0);
    dprintf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
    dprintf("%d %s(s)%", 0, "message");
    dprintf("\n");
    dprintf("%d %s(s) with %%\n", 0, "message");
    dsprintf(buf, "justif: \"%-10s\"\n", "left"); printf("%s", buf);
    dsprintf(buf, "justif: \"%10s\"\n", "right"); printf("%s", buf);
    dsprintf(buf, " 3: %04d zero padded\n", 3); printf("%s", buf);
    dsprintf(buf, " 3: %-4d left justif.\n", 3); printf("%s", buf);
    dsprintf(buf, " 3: %4d right justif.\n", 3); printf("%s", buf);
    dsprintf(buf, "-3: %04d zero padded\n", -3); printf("%s", buf);
    dsprintf(buf, "-3: %-4d left justif.\n", -3); printf("%s", buf);
    dsprintf(buf, "-3: %4d right justif.\n", -3); printf("%s", buf);

    return 0;
}

/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_PRINTF -c printf.c
 * you will get a normal warning:
 *   printf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */

#endif
