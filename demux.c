
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <assert.h>
#include <stdbool.h>
#include "demux.h"

#define MAX_SLOT_COUNT			32
#define MAX_FILTER_COUNT		32


#define TS_PACKET_SIZE				188
#define TS_SYNC_FLAG						(0x47)

#define FILE_NAME_LEN					256
#define FILE_BUF_SIZE						(2 * 4 * 47 * 51 * 52)

#define AV_RB16(x)                      \
    (((const unsigned char*)(x))[0] << 8 |     \
    ((const unsigned char*)(x))[1])

typedef struct 
{
	//  transport packet specific header; see the MPEG2 systems spec
	//  for descriptions
	unsigned long   sync_byte;
	bool				transport_error_indicator;
	bool				payload_unit_start_indicator;
	bool				transport_priority;
	unsigned long		PID;
	unsigned long		transport_scrambling_control;
	unsigned long		adaptation_field_control;
	unsigned long		continuity_counter;

    char		*pbSysBuffer;													//  points to the start of sys packet
	int			iSysBufferLength;										//  length of sys packet
	char		*pbSysBufferPayload;							//  points to the sys packet payload, following header
	int			iSysBufferPayloadLength;				//  length of payload
} MpegTSPacket_t;





typedef struct{
	int file_len;
	char *file_buf;
	long left_size;
}dmx_handle_t;


int find_next_packet(char *p_buf, long len, long *p_offset){
	*p_offset = 0;
	while (1){
		if (len - *p_offset <= TS_PACKET_SIZE){
			return -1;
		}

		if (p_buf[*p_offset] == TS_SYNC_FLAG && p_buf[*p_offset + TS_PACKET_SIZE] == TS_SYNC_FLAG){
			return 0;
		}else{
			*p_offset += 1;
		}
	}
	return -1;
}

int parse_raw_packet_data(char *p_raw_pkt, MpegTSPacket_t *p_ts_pkt)
{
	char *p, *p_end;
	
	// init
	memset(p_ts_pkt, 0, sizeof(MpegTSPacket_t));		

	// parse
	p_ts_pkt->sync_byte														= p_raw_pkt[0];
	p_ts_pkt->transport_error_indicator						= (p_raw_pkt[1] & 0x80 ? true : false);						
	p_ts_pkt->payload_unit_start_indicator				= (p_raw_pkt[1] & 0x40 ? true : false);
	p_ts_pkt->transport_priority									= (p_raw_pkt[1] & 0x20  ? true : false);
	p_ts_pkt->PID																	= AV_RB16(p_raw_pkt + 1) & 0x1fff;
	p_ts_pkt->transport_scrambling_control				= (p_raw_pkt[3] >> 6) & 0x03;
	p_ts_pkt->adaptation_field_control						= (p_raw_pkt[3] >> 4) & 0x03;
	p_ts_pkt->continuity_counter									= (p_raw_pkt[3] >> 0) & 0x0f;		

	/* skip adaptation field */		
	p = p_raw_pkt + 4;
	if (p_ts_pkt->adaptation_field_control == 0) /* reserved value */
	{		
		return -1;
	}
	else if (p_ts_pkt->adaptation_field_control == 2) /* adaptation field only */
	{		
		return -1;
	}
	if (p_ts_pkt->adaptation_field_control == 3)				/* payload follows adapation field */
	{		
		p += p[0] + 1;
	}
	
	/* if past the end of p_raw_pkt, ignore */
	p_end = p_raw_pkt + 188;
	
	if (p >= p_end)
	{		
		return -1;
	}		

	p_ts_pkt->pbSysBuffer = p_raw_pkt;
	p_ts_pkt->iSysBufferLength = 188;

	p_ts_pkt->pbSysBufferPayload = p;
	p_ts_pkt->iSysBufferPayloadLength = (p_end - p);	

	return 0;
}


bool check_mask(char *pData, char *code, char *mask)
{		
	for(int i = 0; i < FILTER_CODE_LEN; i++)
	{
		if((pData[i] & mask[i]) != (code[i] & mask[i]))
		{
			return false;
		}
	}
	return true;	
}

int push_section_data(MpegTSFilter_t *p_filter, char *p_buf, int len, bool is_start)
{
	MpegTSSectionFilter_t		*section_filter = &p_filter->secFilter;	

	if (is_start) 
	{
		memcpy(p_filter->pBuf, p_buf, len);
		section_filter->section_index = len;
		section_filter->section_h_size = -1;		
	}
	else 
	{
		int wanted = p_filter->BufLen  - section_filter->section_index;
		if (len < wanted)
		{
			wanted = len;
		}
		memcpy(p_filter->pBuf + section_filter->section_index, p_buf, wanted);
		section_filter->section_index += wanted;
	}

	/* compute section length if possible */
	if (section_filter->section_h_size == -1 && section_filter->section_index >= 3)
	{
		len = (AV_RB16(p_filter->pBuf + 1) & 0xfff) + 3;
		if (len > 4096)
		{
			// wrong section, drop it
			// reset p_filter	
			p_filter->pDataLen = 0;
			p_filter->pDataOk = false;
			p_filter->last_cc = -1;

			section_filter->section_index = 0;
			section_filter->section_h_size = -1;

			return -1;
		}

		section_filter->section_h_size = len;
	}

	if (section_filter->section_h_size != -1 && section_filter->section_index >= section_filter->section_h_size)
	{	
		/*
		if (p_filter->b_crc_enable)
		{
			if (av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1, p_filter->pBuf, section_filter->section_h_size) != 0)
			{
				*(p_filter->pDataLen) = 0;
				*(p_filter->pDataOk) = false;
				p_filter->last_cc = -1;

				section_filter->section_index = 0;
				section_filter->section_h_size = -1;
				return 0;
			}
		}
		*/		
		
		if (check_mask(p_filter->pBuf, p_filter->Code, p_filter->Mask))
		{
			p_filter->pDataLen = section_filter->section_h_size;
			p_filter->pDataOk  = true;
			
			section_filter->section_index = 0;
			section_filter->section_h_size = -1;
      		return 0;
		}		
		else
		{
			p_filter->pDataLen = 0;
			p_filter->pDataOk = false;
			p_filter->last_cc = -1;

			section_filter->section_index = 0;
			section_filter->section_h_size = -1;
		}		
	}

	return 0;
}


int handle_section( MpegTSFilter_t *p_filter, MpegTSPacket_t *p_ts_pkt )
{
	// section
	char *p, *p_end;
	int len;
	bool cc_ok;		

	p = p_ts_pkt->pbSysBufferPayload;
	p_end = p + p_ts_pkt->iSysBufferPayloadLength;

	cc_ok = (p_filter->last_cc < 0) || (((p_filter->last_cc + 1) & 0x0f) == p_ts_pkt->continuity_counter);
	if (p_ts_pkt->payload_unit_start_indicator)
	{
		/* pointer field present */
		len = *p++;
		if (p + len > p_end)
		{
			return -1;
		}

		if (len && cc_ok)
		{
			/* write remaining section bytes */
			push_section_data(p_filter, p, len, 0);
		}
		
		p += len;
		if (p < p_end)
		{
			push_section_data(p_filter, p, p_end - p, 1);
		}
	}
	else
	{
		if (cc_ok)
		{
			push_section_data(p_filter, p, p_end - p, 0);
		}
	}
	return 0;
}




int handle_raw_packet_data(char *p_raw_pkt, MpegTSFilter_t *filter)
{	
	MpegTSPacket_t ts_pkt;	

	if (parse_raw_packet_data(p_raw_pkt, &ts_pkt) != 0)
	{		
		return -1;
	}


    if (filter->PID != ts_pkt.PID)
    {
        return -1;
    }			

    					
	return handle_section(filter, &ts_pkt);
}


int dmx_process_filter(void *h, MpegTSFilter_t *filter)
{  
    char *p_buf     = NULL;
    int proc_size = 0;

	dmx_handle_t *pthis = (dmx_handle_t *)h;

	p_buf = pthis->file_buf;

    while (proc_size <= pthis->file_len)
    {
        int  ret;
        long offset;

        ret = find_next_packet(p_buf, pthis->left_size, &offset);
        p_buf += offset;
        pthis->left_size -= offset;
        proc_size += TS_PACKET_SIZE;
        
        if (ret == 0)
        {
            int ret = handle_raw_packet_data(p_buf, filter);
            p_buf += TS_PACKET_SIZE;
            pthis->left_size -= TS_PACKET_SIZE;

            if(filter->pDataOk){
				return 0;
			}
        }else{
            break;
        }

        if(pthis->left_size <= TS_PACKET_SIZE){
            pthis->left_size = pthis->file_len;
        }
    }

	return -1;
}

void * dmx_init_handle(char *path){
	int fd;
    struct stat file_info;

	dmx_handle_t *pthis = malloc(sizeof(dmx_handle_t));

    // 打开文件
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open the file");
        return NULL;
    }

    // 获取文件大小
    if (fstat(fd, &file_info) == -1) {
        perror("Failed to get file size");
        close(fd);
        return NULL;
    }
    off_t file_size = file_info.st_size;

	if(file_size >= 20971400) {
		file_size = 20971400;
	}

    // 分配足够大小的内存
    pthis->file_buf = (char*)malloc(file_size);
    if (pthis->file_buf == NULL) {
        perror("Failed to allocate memory");
        close(fd);
        return NULL;
    }

    // 读取文件内容到内存
    ssize_t bytes_read = read(fd, pthis->file_buf, file_size);
    if (bytes_read != file_size) {
        perror("Failed to read the file");
        free(pthis->file_buf);
        close(fd);
        return NULL;
    }

    // 输出读取到的内容
    printf("Read %zd bytes:\n", bytes_read);
    //printf("%.*s", (int)bytes_read, pthis->file_buf);

    // 释放内存并关闭文件
    pthis->file_len = file_size;
	pthis->left_size = file_size;
    close(fd);

	return (void *)pthis;
}


void  dmx_deinit_handle(char *h){
	dmx_handle_t *pthis = (dmx_handle_t *)h;
	free(pthis->file_buf);
	free(pthis);
}
