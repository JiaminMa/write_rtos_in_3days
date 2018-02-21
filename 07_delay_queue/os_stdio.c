#include <stdarg.h>
#include "os_stdio.h"

volatile uint32_t * const UART0DR = (uint32_t *)0x4000C000;

char send_char(uint8_t *ch)
{
    *UART0DR = *ch;
    return *ch;
}

const char hex_asc_table[16] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
const char upper_hex_asc_table[16] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

int is_dec_asc(char ch)
{
    uint32_t i;
    for (i = 0; i < 10; i++) {
        if (ch == hex_asc_table[i])
            return 1;
    }

    return 0;
}

int is_asc(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

int is_hex_asc(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

int printf_char(char ch)
{
    uint8_t c1 = (uint8_t)'\r';
    uint8_t c2 = 0;

    c2 = (uint8_t)ch;

    send_char(&c2);

    if (((uint8_t)'\n') == c2) {
        send_char(&c1);
    }

    return 0;
}

int printf_str(char *str)
{
    while (str && (*str != (char)'\0')) {
        printf_char(*str);
        str++;
    }

    return 0;
}

int printf_hex(uint32_t val, uint32_t width)
{
    int i = 0;
    char hex_val = 0, asc = 0;

    if ((width > 8) || (width == 0))
        width = 8;

    for (i = width - 1; i >= 0; i--) {
        hex_val = (val & (0x0F << (i << 2))) >> (i << 2);
        asc = hex_asc_table[(int)hex_val];
        printf_char(asc);
    }

    return 0;
}

int printf_hex_upper(uint32_t val, uint32_t width)
{
    int i = 0;
    char hex_val = 0, asc = 0;

    if ((width > 8) || (width == 0))
        width = 8;

    for (i = width - 1; i >= 0; i--) {
        hex_val = (val & (0x0F << (i << 2))) >> (i << 2);
        asc = upper_hex_asc_table[(int)hex_val];
        printf_char(asc);
    }

    return 0;
}

#if defined(MULTIPLY_DIVIDE_CAN_BE_USED)
int printf_dec(uint32_t val)
{
    uint8_t buf[16];
    char asc = 0;
    int i = 0;
    while (1) {
        buf[i] = val % 10;
        val = val / 10;
        i++;
        if (val == 0) {
            break;
        }
    }

    for (; i > 0; i--) {
        asc = hex_asc_table[buf[i - 1]];
        printf_char(asc);
    }

    return 0;
}
#else
const uint32_t hex_weight_value_table[] =
    { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

int printf_dec(uint32_t val)
{
    uint32_t i = 0;
    uint32_t tmp = 1, tmp_w = 1;
    char asc = 0;

    /* Figure out the digitals */
    while (1) {
        tmp = (tmp << 3) + (tmp << 1);  // tmp *= 10;
        i++;
        if (tmp > val) {
            break;
        }
    }

    if (i > 8) {
        return -1;
    }

    while (i > 0) {
        if (val >= ((hex_weight_value_table[i - 1] << 3) + hex_weight_value_table[i - 1])) {    //<=9xxx
            tmp = 9;
            tmp_w = (hex_weight_value_table[i - 1] << 3) + hex_weight_value_table[i - 1];
        } else if (val >= (hex_weight_value_table[i - 1] << 3)) {   //8xxx
            tmp = 8;
            tmp_w = hex_weight_value_table[i - 1] << 3;
        } else if (val >=
                   ((hex_weight_value_table[i - 1] << 2) + (hex_weight_value_table[i - 1] << 1) +
                    hex_weight_value_table[i - 1])) {
            tmp = 7;
            tmp_w =
                (hex_weight_value_table[i - 1] << 2) + (hex_weight_value_table[i - 1] << 1) +
                hex_weight_value_table[i - 1];
        } else if (val >=
                   ((hex_weight_value_table[i - 1] << 2) + (hex_weight_value_table[i - 1] << 1))) {
            tmp = 6;
            tmp_w = (hex_weight_value_table[i - 1] << 2) + (hex_weight_value_table[i - 1] << 1);
        } else if (val >= ((hex_weight_value_table[i - 1] << 2) + hex_weight_value_table[i - 1])) {
            tmp = 5;
            tmp_w = (hex_weight_value_table[i - 1] << 2) + hex_weight_value_table[i - 1];
        } else if (val >= (hex_weight_value_table[i - 1] << 2)) {
            tmp = 4;
            tmp_w = hex_weight_value_table[i - 1] << 2;
        } else if (val >= ((hex_weight_value_table[i - 1] << 1) + hex_weight_value_table[i - 1])) {
            tmp = 3;
            tmp_w = (hex_weight_value_table[i - 1] << 1) + hex_weight_value_table[i - 1];
        } else if (val >= (hex_weight_value_table[i - 1] << 1)) {
            tmp = 2;
            tmp_w = hex_weight_value_table[i - 1] << 1;
        } else if (val >= (hex_weight_value_table[i - 1])) {
            tmp = 1;
            tmp_w = hex_weight_value_table[i - 1];
        } else {
            tmp = 0;
            tmp_w = 0;
        }

        asc = hex_asc_table[tmp];
        printf_char(asc);
        i--;

        val -= tmp_w;
    }

    return 0;
}
#endif

void no_printk(const char *fmt, ...)
{

}

void printk(const char *fmt, ...)
{
    char c;
    uint32_t width = 0;
    va_list argptr;

    va_start(argptr, fmt);
    do {
        c = *fmt;
        if (c != '%') {
            printf_char(c);
        } else {
            while (1) {
                c = *++fmt;
                if ((c == 'd') || (c == 'x') || (c == 'X') || (c == 's') || (c == 'c')) {
                    if ((c == 'x') || (c == 'X')) {
                        if (*(fmt - 1) == '%')
                            width = 8;
                        else
                            width = *(fmt - 1) - '0';
                    }
                    break;
                }
            }

            switch (c) {
            case 'd':
                printf_dec(va_arg(argptr, int));
                break;
            case 'x':
                printf_hex((va_arg(argptr, int)), width);
                break;
            case 'X':
                printf_hex_upper((va_arg(argptr, int)), width);
                break;
            case 's':
                printf_str(va_arg(argptr, char *));
                break;
            case 'c':
                printf_char(va_arg(argptr, int));
                break;
            default:
                break;
            }
        }
        ++fmt;
    }
    while (*fmt != '\0');

    va_end(argptr);
}

int memset(void *mem, uint8_t val, uint32_t sz)
{
    uint8_t *p = (uint8_t *)mem;
    int i = 0;

    for (i = 0; i < sz; i++, *p++ = val) ;

    return 0;
}

int memcpy(void *dst, const void *src, uint32_t sz)
{
    uint8_t *p_dst = (uint8_t *)dst;
    uint8_t *p_src = (uint8_t *)src;
    int i = 0;

    for (i = 0; i < sz; i++, *p_dst++ = *p_src++) ;

    return 0;
}

int memcmp(void *mem1, void *mem2, uint32_t sz)
{
    uint8_t *p_mem1 = (uint8_t *)mem1;
    uint8_t *p_mem2 = (uint8_t *)mem2;
    int i = 0;

    for (i = 0; i < sz; i++, p_mem1++, p_mem2++) {
        if (*p_mem1 != *p_mem2) {
            break;
        }
    }

    if (i < sz) {
        return 1;
    } else {
        return 0;
    }
}

int strcmp(char *str1, char *str2)
{
    char *p1 = str1, *p2 = str2;

    while ((*p1 != 0) && (*p2 != 0) && (*p1 == *p2)) {
        p1++;
        p2++;
    }

    return *p1 - *p2;
}

int strncmp(char *str1, char *str2, uint32_t sz)
{
    char *p1 = str1, *p2 = str2;
    int i = 0;

    for (; i < sz; i++, p1++, p2++) {
        if (*p1 > *p2) {
            return 1;
        }
        if (*p1 < *p2) {
            return -1;
        }
    }

    return 0;
}

int strtoul(char *str, uint32_t *val)
{
    char *p = str;
    uint32_t multiplier = 10;
    uint32_t result = 0, num = 0;

    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
        multiplier = 0x10;
    }

    while (0 != *p) {
        if (multiplier == 0x10) {
            if (!is_hex_asc(*p)) {
                return -1;
            }
            if (*p >= '0' && *p <= '9') {
                num = *p - '0';
            } else if (*p >= 'a' && *p <= 'f') {
                num = *p - 'a' + 10;
            } else {
                num = *p - 'A' + 10;
            }
        } else {
            if (!is_dec_asc(*p)) {
                return -2;
            }

            num = *p - '0';
        }

        result = result * multiplier + num;

        p++;
    }

    *val = result;

    return 0;
}

int strtol(char *str, int *val)
{
    char *p = str;
    int ret;

    if (*p == '-') {
        ret = strtoul(++p, (uint32_t *)val);
        *val = -(*val);
        return ret;
    } else
        return strtoul(p, (uint32_t *)val);

}

uint32_t strlen(char *str)
{
    char *p = str;

    while (*p++ != '\0') ;

    return (uint32_t)(p - str);
}

int strcpy(char *dst, char *src)
{
    char *p_dst = dst, *p_src = src;

    while (*p_src != '\0') {
        *p_dst = *p_src;
        p_dst++;
        p_src++;
    }
    *p_dst = '\0';

    return 0;
}
