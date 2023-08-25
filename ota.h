#ifndef _RL_OTA__H_
#define _RL_OTA__H_

typedef enum {
    RLOTA_MSG_DOWNLOAD,
}rlota_state_t;





typedef int (*rlota_cb)(rlota_state_t state, int param, int percent, int data1,int data2);

#endif