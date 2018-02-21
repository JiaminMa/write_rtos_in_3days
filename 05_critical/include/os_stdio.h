#ifndef OS_STDIO_H
#define OS_STDIO_H

#include <stdint.h>

extern void printk(const char *fmt, ...);
extern int memset(void *mem, uint8_t val, uint32_t sz);
extern int memcpy(void *dst, const void *src, uint32_t sz);
extern int memcmp(void *mem1, void *mem2, uint32_t sz);
extern int strcmp(char *str1, char *str2);
extern int strncmp(char *str1, char *str2, uint32_t sz);
extern int strtoul(char *str, uint32_t *val);
extern int strtol(char *str, int *val);
extern uint32_t strlen(char *str);
extern int strcpy(char *dst, char *src);
extern void no_printk(const char *fmt, ...);

#define DEBUG
#ifdef DEBUG
#define DEBUG printk
#else
#define DEBUG no_printk
#endif /*DEBUG*/

#endif /*OS_STDIO_H*/
