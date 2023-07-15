#include "flap_utils.h"

int strLen_utf8(char *s){
    int i = 0, j = 0;
    while(s[i]){
        if ((s[i] & 0xC0) != 0x80) j++;
        i++;
    }
    return j;
}

int charLen_utf8(char *s){
    if((*s & 0xF7) == 0xF0) return 4;
    if((*s & 0xF0) == 0xE0) return 3;
    if((*s & 0xE0) == 0xC0) return 2;
    return 1;
}

char * strIndex_utf8(char *s, int index){
    int i = 0, j = 0;
    while(s[i] || j<index){
        if ((s[i] & 0xC0) != 0x80) j++;
        i++;
    }
    return &s[i];
}

char * strNext_utf8(char *s){
    int i;
    for(i = 1;s[i] && (s[i] & 0xC0) == 0x80;i++);
    return s+i;
}