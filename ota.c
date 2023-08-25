#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "rl_head.h"
#include "rl_dmx.h"
#include "rl_util.h"
#include "ota.h"

#define RLOTA_RED_MODE
#define RED_OTA_OUI     (0x000902)
//#define RED_OTA_PID     (110)

#define SW_MODULE_ID    (100)
#define SW_MODULE_NAME  "XXX_SW"
#define SW_HW_MAJVER    (0xAA)
#define SW_HW_MINVER    (0x01)

#define DB_MODULE_ID    (101)
#define DB_MODULE_NAME  "XXX_DB"
#define DB_HW_MAJVER    (0xAA)
#define DB_HW_MINVER    (0x01)

#define EMU_MODULE_ID   (102)
#define EMU_MODULE_NAME "XXX_EMU"
#define EMU_HW_MAJVER   (0xAA)
#define EMU_HW_MINVER   (0x01)

#define RLOTA_MAX  2


#define otadbg printf

typedef struct
{
	#define RLOTA_MAX_MODULE_NAME_LEN 16
	unsigned int   OUI;
	unsigned short module_id;
	unsigned short hw_module_id;
	unsigned short hw_module_version;
	unsigned short sw_module_id;
	unsigned short sw_module_version;
	char           module_name[RLOTA_MAX_MODULE_NAME_LEN];
}rlota_module_box_t;

typedef struct
{
	unsigned short sw_module_version;
	unsigned char  group_id;
}rlota_module_stream_t;

typedef struct
{
	rlota_module_box_t module;
	rlota_module_stream_t module_stream;
}rlota_module_ex_t;

typedef struct
{
	int match_module_idx;
	int match_module_new_version;
	rlota_module_ex_t modules[RLOTA_MAX];
	
	unsigned short ssu_pid;
	unsigned int   dsi_to_ms;
	unsigned int   dii_to_ms;
	unsigned int   ddb_to_ms;
	RL_HANDLE      dmx;
	
	unsigned int   command;
	unsigned int   cancel_dmx;
	unsigned char  new_cmd;
	unsigned char  running;

	unsigned char *buf;
	unsigned int   buf_sz;
	unsigned char *block_flag;
	unsigned int   block_num;
	unsigned int   block_downloaded;
	unsigned int   downloaded_size;
	
	rlota_cb cb;
	void *cb_usrdata;
}rlota_t;


static int rlota_red_parse_dii(rlota_t *ota, unsigned char *sect, unsigned int len, unsigned int transaction_id){
	int i, module_idx, module_num;
	const unsigned char *p_data;
	const unsigned char *p_end;
	unsigned char  group_id;
	unsigned short sect_len,msg_len;
	unsigned int OUI;
	
	if(ota == RL_NULL || sect == RL_NULL || len == 0){
		return RLOTA_E_BAD_PARAMS;
	}

	p_data = sect;
	if(p_data[0] != 0x3B) {
		return RLOTA_E_TRY_NEXT;
	}
	sect_len = ((p_data[1]&0x0F) << 8) | p_data[2];
	if((sect_len+3) != len){
		return RLOTA_E_TRY_NEXT;
	}
	p_end = sect + sect_len + 3 - 4;
	
	group_id = p_data[3];
	msg_len = (p_data[18]<<8) |(p_data[19]);
	OUI = (p_data[43]<<16) | (p_data[44]<<8) | p_data[45];

	module_num = p_data[52];
	p_data += 54;

	int no_new_version = 0;
	char module_name[RLOTA_MAX_MODULE_NAME_LEN] = {0};
	for(module_idx = 0; p_data < p_end && module_idx < module_num; module_idx++) {
		unsigned char  module_info_len = p_data[6];
		unsigned char  module_id = p_data[0];
		unsigned short sw_version = (p_data[7]<<8)|p_data[8];
		unsigned short hw_version = (p_data[9]<<8)|p_data[10];
		rl_strcpy_s(module_name, sizeof(module_name), (char*)p_data+11);
		otadbg("[%d/%d] module_id=%02X, swver=%04X, hwver=%04X\n", module_idx, module_num, module_id, sw_version, hw_version);

		for(i = 0; i < RLOTA_MAX; i++){
			rlota_module_ex_t *module_ex = ota->modules + i;
			rlota_module_box_t *module = &module_ex->module;
			if(module->OUI == 0)
				continue;
			
			if(module->OUI != OUI || module->module_id != module_id 
				|| module->hw_module_version != hw_version
				|| strcmp(module->module_name, module_name)){
				otadbg("[%d-%d] unmatch: OUI=%X-%X, moudleid=%d-%d, hw-ver=%d-%d, name=%s--%s\n", module_idx, i, OUI, module->OUI, module_id, module->module_id, hw_version, module->hw_module_version, module_name, module->module_name);
				continue;
			}
			
			module_ex->module_stream.sw_module_version = sw_version;
			module_ex->module_stream.group_id = group_id;
			otadbg("[%d-%d] match: OUI=%X-%X, moudleid=%d-%d, hw-ver=%d-%d, name=%s--%s\n", module_idx, i, OUI, module->OUI, module_id, module->module_id, hw_version, module->hw_module_version, module_name, module->module_name);
			otadbg("[%d-%d] match: match_module_idx=%d\n", module_idx, i, ota->match_module_idx);
			otadbg("[%d-%d] match: group_id=%d\n", module_idx, i, group_id);
			otadbg("[%d-%d] match: version=%d-%d\n", module_idx, i, sw_version, module->sw_module_version);
			if(sw_version > module->sw_module_version){
				ota->match_module_idx = i;
				ota->match_module_new_version = sw_version;
				return RLOTA_SUCCESS;
			}

			if(sw_version <= module->sw_module_version){
				//return RLOTA_E_NO_NEW_VERSION;
				no_new_version = 1;
			}
		}

		p_data += (8 + module_info_len);
	}

	if(no_new_version == 1){
		return RLOTA_E_NO_NEW_VERSION;
	}

	return RLOTA_E_NOT_FOUND;
}


static int rlota_red_parse_ddb(rlota_t *ota, unsigned char *p_sect,unsigned int len){
	unsigned char *p_data;
	unsigned short sect_len;

	p_data = p_sect;
	if(p_data[0] != 0x3C){
		otadbg("table=%02X\n", p_data[0]);
		return RLOTA_E_TRY_NEXT;
	}

	sect_len = ((p_data[1]&0x0F) << 8) |p_data[2];
	if ((sect_len+3) != len) {
		otadbg("sect_len=%d-%d\n", sect_len+3, len);
		return RLOTA_E_TRY_NEXT;
	}

	unsigned int crc1 = rl_crc32_mpeg2(p_sect, sect_len+3-4);
	unsigned int crc2 = (p_data[len-4]<<24) | (p_data[len-3]<<16) | (p_data[len-2]<<8) | (p_data[len-1]);
	if(crc1 != crc2){
		otadbg("crc=%x-%x\n", crc1, crc2);
		return RLOTA_E_TRY_NEXT;
	}
	
	unsigned char  group_id = p_data[12];
	unsigned char  module_id = p_data[4];
	unsigned short sw_version = (p_data[14]<<8)|p_data[15];
	unsigned short block_id = (p_data[6]<<8)|p_data[7];
	unsigned short block_total = (p_data[28]<<8)|p_data[29];

	rlota_module_ex_t *module_ex = ota->modules + ota->match_module_idx;
	if(group_id != module_ex->module_stream.group_id){
		otadbg("group_id=%d-%d\n", group_id, module_ex->module_stream.group_id);
		return RLOTA_E_TRY_NEXT;
	}

	if(module_id != module_ex->module.module_id){
		otadbg("module_id=%d-%d\n", module_ex, module_ex->module.module_id);
		return RLOTA_E_TRY_NEXT;
	}

	if(sw_version != module_ex->module_stream.sw_module_version){
		otadbg("sw_version=%d-%d\n", sw_version, module_ex->module_stream.sw_module_version);
		return RLOTA_E_UNKNOWN;
	}

	if(ota->block_flag == RL_NULL){
		ota->block_flag = rl_malloc_zero(block_total);
		if(ota->block_flag == RL_NULL)
			return RLOTA_E_OUT_OF_MEM;
		ota->block_num = block_total;
		ota->block_downloaded = 0;
	}

	if(block_id >= block_total || block_total > ota->block_num){
		otadbg("block=%d-%d-%d\n", block_id, block_total, ota->block_num);
		return RLOTA_E_TRY_NEXT;
	}

	if(ota->block_flag[block_id] == 1){
		return RLOTA_E_TRY_NEXT;
	}
	
	p_data += 30;
	int payload_len = sect_len -(30+4-3);
	if (payload_len <= 0) {
		return RLOTA_E_TRY_NEXT;
	}

	#define MAX_DDB_SECTION_SIZE (4062)
	memcpy(ota->buf + block_id * MAX_DDB_SECTION_SIZE, p_data, /*MAX_DDB_SECTION_SIZE*/payload_len);
	ota->block_flag[block_id] = 1;
	ota->block_downloaded++;
	ota->downloaded_size += payload_len;
	//otadbg("group_id=%02X, module_id=%02X, sw_ver=%04X, blk=%d/%d-%d, %d\n", group_id, module_id, sw_version, block_id, block_total, ota->block_downloaded, ota->downloaded_size);
	if(ota->block_downloaded == ota->block_num){
		//ota->downloaded_size = MAX_DDB_SECTION_SIZE * ota->block_num;
		ota->downloaded_size--; //redline OTA stream tool read file EOF, we dont need
		otadbg("group_id=%02X, module_id=%02X, sw_ver=%04X, blk=%d/%d-%d, %d\n", group_id, module_id, sw_version, block_id, block_total, ota->block_downloaded, ota->downloaded_size);
		otadbg("finish\n");
		return RLOTA_SUCCESS;
	}

	return RLOTA_E_TRY_NEXT;
}

static int rlota_red_search(rlota_t *ota){
	unsigned int group_id = 0;
	int ret = 0, i, j, match = 0; (void)j; (void)match;
	RL_U8 *sect = RL_NULL;
	RL_U16 len = 0;
	unsigned int param = 0;
	
	ota->match_module_idx = -1;
	//for(i = 0; i < RLOTA_MAX; i++){
	//	memset(&ota->modules[i].module_stream, 0, sizeof(rlota_module_stream_t));
	//}

    memset(&ota->modules[0].module_stream, 0, sizeof(rlota_module_stream_t));
    ota->modules[0].module.OUI = 0x000902;
	ota->modules[0].module.module_id = 0x90; //???//0xA0; //b11590
    ota->modules[0].module.hw_module_version = 0xAA01;
    ota->modules[0].module.sw_module_version = 0x270f;
    strcpy(ota->modules[0].module.module_name, "flash");

    ota->modules[1].module.OUI = 0x000902; //5c32336d
	ota->modules[1].module.module_id = 0xA1;
    ota->modules[1].module.hw_module_version = 0xAA01;
    ota->modules[1].module.sw_module_version = 0x0001;
    strcpy(ota->modules[1].module.module_name, "preset_tkgs");

	rlota_filt_t filt;
	
	rl_memset(&filt, 0, sizeof(rlota_filt_t));
	filt.match[0] = 0x3B;
	filt.mask[0]  = 0xFF;
	filt.match[6] = 0x00;
	filt.mask[6]  = 0xFF;
	filt.match[8] = 0x11;
	filt.mask[8]  = 0xFF;
	filt.match[9] = 0x03;
	filt.mask[9]  = 0xFF;
	filt.match[10]= 0x10;
	filt.mask[10] = 0xFF;
	filt.match[11]= 0x02;
	filt.mask[11] = 0xFF;
	filt.len = 12;

	ret = rlota_dmx_set_filt(&ota->dmx, ota->ssu_pid, &filt, 1, ota);
	if(ret != 0)
	{
		otadbg("ret=%d\n", ret);
		return RLOTA_E_DEMUX;
	}

	unsigned int tm0 = rl_get_time_ms();
	while(1)
	{
		ret = rlota_dmx_collect(ota->dmx, &ota->cancel_dmx, ota->dii_to_ms, &sect, &len, &param);
		if(ret != RLOTA_SUCCESS)
		{
			otadbg("ret=%d\n", ret);
			rlota_dmx_close_filt(ota->dmx);
			return ret;
		}
		ret = rlota_red_parse_dii(ota, sect, len, group_id);
		otadbg("ret=%d, match_module_idx=%d\n", ret, ota->match_module_idx);
		rlota_dmx_release(ota->dmx, param);
		if(ret == RLOTA_E_NO_NEW_VERSION || ret == RLOTA_SUCCESS)
		{
			rlota_dmx_close_filt(ota->dmx);
			return ret;
		}
		if(ota->dii_to_ms != 0 && rl_get_time_ms() > tm0 + ota->dii_to_ms)
		{
			rlota_dmx_close_filt(ota->dmx);
			return RLOTA_E_TIMEOUT;
		}
	}
	
	return ret;
}




int g_rlota_search_test(short pid){
    rlota_t param;

    memset(&param, 0, sizeof(rlota_t));
    param.ssu_pid = pid;

    return rlota_red_search(&param);
}
