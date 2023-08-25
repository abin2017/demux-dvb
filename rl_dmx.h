#ifndef		_RL_DEMUX_H
#define	    _RL_DEMUX_H


typedef struct{
    RL_U8 match[16];
    RL_U8 mask[16];
    RL_U8 len;
}rlota_filt_t;



int rlota_dmx_set_filt(RL_HANDLE * h, short pid, rlota_filt_t *p_filter, RL_U32 b_circle, RL_U32 param);

int rlota_dmx_collect(RL_HANDLE h, unsigned int *b_cancel, unsigned int timeout, RL_U8 **p_buffer, RL_U16 *len, RL_U32 *param);

int rlota_dmx_close_filt(RL_HANDLE h);

int rlota_dmx_release(RL_HANDLE h, RL_U32 param);

#endif