#ifndef		_RL_UTIL_H
#define	    _RL_UTIL_H


unsigned int rl_get_time_ms();

unsigned int rl_crc32_mpeg2(char *in, int len);

unsigned char * rl_malloc_zero(int size);

#endif