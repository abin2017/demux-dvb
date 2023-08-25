#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "rl_head.h"
#include "rl_dmx.h"
#include "demux.h"

#define FILTER_BUFFER_LEN (4 * 1024)

typedef struct{
    void * h;
    MpegTSFilter_t filter;
    char buffer[FILTER_BUFFER_LEN];
}rl_dmx_t;

int rlota_dmx_set_filt(RL_HANDLE * handle, short pid, rlota_filt_t *p_filter, RL_U32 b_circle, RL_U32 param){
    MpegTSFilter_t *p_filt;
    void * h = NULL; 
    rl_dmx_t *rh = malloc(sizeof(rl_dmx_t));
    int i = 0;

    h = dmx_init_handle("../redline.trp");

    p_filt = &rh->filter;
    rh->h = h;

    memset(p_filt, 0, sizeof(MpegTSFilter_t));

    for(i = 0; i < p_filter->len; i ++){
        p_filt->Code[i] = p_filter->match[i];
        p_filt->Mask[i] = p_filter->mask[i];
    }

    p_filt->pBuf = rh->buffer;
    p_filt->BufLen = FILTER_BUFFER_LEN;
    p_filt->PID = pid;

    *handle = (RL_HANDLE *)rh;

    return RLOTA_SUCCESS;
}

int rlota_dmx_collect(RL_HANDLE h, unsigned int *b_cancel, unsigned int timeout, RL_U8 **p_buffer, RL_U16 *len, RL_U32 *param){
    rl_dmx_t *rh = (rl_dmx_t *)h;
    MpegTSFilter_t *p_filter = &rh->filter;
    char *pdata = NULL;

    p_filter->pDataLen = 0;
    p_filter->pDataOk = 0;
    
    if(dmx_process_filter(rh->h, p_filter) == 0){
        if(p_filter->pDataOk){
            pdata =  malloc(p_filter->pDataLen + 1);
            memcpy(pdata, p_filter->pBuf, p_filter->pDataLen);
            pdata[p_filter->pDataLen] = 0;
            *len = p_filter->pDataLen;
            *param = (RL_U32)pdata;
            *p_buffer = pdata;
            return RLOTA_SUCCESS;
        }
    }

    return RLOTA_E_TIMEOUT;
}

int rlota_dmx_close_filt(RL_HANDLE h){
    rl_dmx_t *rh = (rl_dmx_t *)h;

    if(rh){
        dmx_deinit_handle(rh->h);
        
        free(rh);
    }
}

int rlota_dmx_release(RL_HANDLE h, RL_U32 param){
    if(param){
        free((void *)param);
    }
}