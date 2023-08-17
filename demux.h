#ifndef		_DEMUX_H
#define	    _DEMUX_H

#define FILTER_CODE_LEN     16

typedef struct 
{
	int section_index;
	int section_h_size;	
}MpegTSSectionFilter_t;

typedef struct MpegTSFilter_t
{
    int PID;
	/*!
	data code
	*/
	char							Code[FILTER_CODE_LEN];
	/*!
	data mask
	*/
	char							Mask[FILTER_CODE_LEN];	
	/*!
	circel buffer
	*/
	bool							CircleBuf;
	/*!
	crc enable flag
	*/
	bool 							b_crc_enable;
	/*!
	buffer base pointer
	*/
	char							*pBuf;
	/*!
	buffer length
	*/
	unsigned int					BufLen;
	/*!
	buffer end pointer
	*/
	unsigned int					pDataLen;
	/*!
	buffer ready flag
	*/
	bool						 	pDataOk;
	/*!
	last continuty count (-1 if first packet)
	*/
	int									last_cc;

	/*!
	filter
	*/
	MpegTSSectionFilter_t       secFilter;
}MpegTSFilter_t;


int dmx_process_filter(void *h, MpegTSFilter_t *filter);

void * dmx_init_handle(char *path);

void  dmx_deinit_handle(char *h);



#endif