#include <stdint.h>
volatile uint32_t * const UART0DR = (uint32_t *)0x4000C000;

void send_str(char *s)
{
    while(*s != '\0') {
        *UART0DR = *s++;
    }
}

void main()
{
    send_str("hello world\n");
    while(1);
}
