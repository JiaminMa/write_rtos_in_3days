#ifndef OS_H
#define OS_H

#include <stdint.h>
typedef enum error_type_enum {
    NO_ERROR = 0,
    ERROR_TIMEOUT = 1,
    ERROR_DEL = 2,
    ERROR_RESOURCE_FULL,
    ERROR_NOT_OWNER,
}error_e;

#endif /*OS_H*/
