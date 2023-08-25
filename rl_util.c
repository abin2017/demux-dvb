#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "rl_head.h"
#include "rl_util.h"

unsigned int rl_get_time_ms(){
    return time(NULL);
}

unsigned int rl_crc32_mpeg2(char *in, int len){
    return 0;
}

unsigned char * rl_malloc_zero(int size){
    unsigned char * p = (unsigned char *)malloc(size);

    if(p){
        memset(p, 0, size);
    }

    return p;
}

void rl_strcpy_s(char *dst, int len, char* src){
    strncpy(dst, src, len);
}

void rl_memset(char *dst, unsigned char val, int size){
    memset(dst, val, size);
}
