#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <linux/can.h>

// lib.c
#define MAX_CANFRAME      "12345678#01.23.45.67.89.AB.CD.EF"

void sprint_canframe(char *buf , struct can_frame *cf, int sep);

void fprint_canframe(FILE *stream , struct can_frame *cf, char *eol, int sep);
// end lib.c

#endif // UTILS_H
