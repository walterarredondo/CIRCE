#include "common.h"


Status get_status(const char *statusStr) {
    if (strcmp(statusStr, "active") == 0) {
        return ACTIVE;
    } else if (strcmp(statusStr, "away") == 0) {
        return AWAY;
    } else if (strcmp(statusStr, "busy") == 0) {
        return BUSY;
    } else {
        return INVALID; // For invalid input
    }
}