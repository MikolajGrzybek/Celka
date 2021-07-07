#include "utils.h"

#include <stdlib.h>
#include <string.h>

// lib.c
void sprint_canframe(char *buf , struct can_frame *cf, int sep) {
    /* documentation see lib.h */

    int i,offset;
    int dlc = (cf->can_dlc > 8)? 8 : cf->can_dlc;

    if (cf->can_id & CAN_ERR_FLAG) {
        sprintf(buf, "%08X#", cf->can_id & (CAN_ERR_MASK|CAN_ERR_FLAG));
        offset = 9;
    } else if (cf->can_id & CAN_EFF_FLAG) {
        sprintf(buf, "%08X#", cf->can_id & CAN_EFF_MASK);
        offset = 9;
    } else {
        sprintf(buf, "%03X#", cf->can_id & CAN_SFF_MASK);
        offset = 4;
    }

    if (cf->can_id & CAN_RTR_FLAG) /* there are no ERR frames with RTR */
        sprintf(buf+offset, "R");
    else
        for (i = 0; i < dlc; i++) {
            sprintf(buf+offset, "%02X", cf->data[i]);
            offset += 2;
            if (sep && (i+1 < dlc))
                sprintf(buf+offset++, ".");
        }
}

void fprint_canframe(FILE *stream , struct can_frame *cf, char *eol, int sep) {
    /* documentation see lib.h */

    char buf[sizeof(MAX_CANFRAME)+1]; /* max length */


    sprint_canframe(buf, cf, sep);
    fprintf(stream, "%s", buf);
    if (eol)
        fprintf(stream, "%s", eol);
}
