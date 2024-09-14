#include <string.h>
typedef enum {
    ACTIVE,
    AWAY,
    BUSY,
    INVALID
} Status;
Status get_status(const char *statusStr);