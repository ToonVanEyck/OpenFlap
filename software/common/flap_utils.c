#include "flap_utils.h"

#include <string.h>

int strLen_utf8(char *s)
{
    int i = 0, j = 0;
    while (s[i]) {
        if ((s[i] & 0xC0) != 0x80)
            j++;
        i++;
    }
    return j;
}

int charLen_utf8(char *s)
{
    if ((*s & 0xF7) == 0xF0)
        return 4;
    if ((*s & 0xF0) == 0xE0)
        return 3;
    if ((*s & 0xE0) == 0xC0)
        return 2;
    return 1;
}

char *strIndex_utf8(char *s, int index)
{
    int i = 0, j = 0;
    while (s[i] || j < index) {
        if ((s[i] & 0xC0) != 0x80)
            j++;
        i++;
    }
    return &s[i];
}

char *strNext_utf8(char *s)
{
    int i;
    for (i = 1; s[i] && (s[i] & 0xC0) == 0x80; i++)
        ;
    return s + i;
}

void picToWord(uint16_t *pic, uint16_t *word)
{
    word[0] = pic[0] | ((pic[7] << 0) & 0xC000);
    word[1] = pic[1] | ((pic[7] << 2) & 0xC000);
    word[2] = pic[2] | ((pic[7] << 4) & 0xC000);
    word[3] = pic[3] | ((pic[7] << 6) & 0xC000);
    word[4] = pic[4] | ((pic[7] << 8) & 0xC000);
    word[5] = pic[5] | ((pic[7] << 10) & 0xC000);
    word[6] = pic[6] | ((pic[7] << 12) & 0xC000);
}

// convert 7 16-bit words to 8 14-bit words
void wordToPic(uint16_t *word, uint16_t *pic)
{
    pic[0] = (word[0] & 0x3FFF);
    pic[1] = (word[1] & 0x3FFF);
    pic[2] = (word[2] & 0x3FFF);
    pic[3] = (word[3] & 0x3FFF);
    pic[4] = (word[4] & 0x3FFF);
    pic[5] = (word[5] & 0x3FFF);
    pic[6] = (word[6] & 0x3FFF);
    pic[7] = ((word[0] & 0xC000) >> 0) | ((word[1] & 0xC000) >> 2) | ((word[2] & 0xC000) >> 4) |
             ((word[3] & 0xC000) >> 6) | ((word[4] & 0xC000) >> 8) | ((word[5] & 0xC000) >> 10) |
             ((word[6] & 0xC000) >> 12);
}