#ifndef FLAP_UTILS_H
#define FLAP_UTILS_H

#include <stdint.h>

int strLen_utf8(char *s);
int charLen_utf8(char *s);
char *strIndex_utf8(char *s, int index);
char *strNext_utf8(char *s);

// convert 8 14-bit words to 7 16-bit words
void picToWord(uint16_t *pic, uint16_t *word);

// convert 7 16-bit words to 8 14-bit words
void wordToPic(uint16_t *word, uint16_t *pic);

#endif