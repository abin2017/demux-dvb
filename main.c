
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "demux.h"

int g_rlota_search_test(short pid);

int main(int argc, char **argv){
	#if 0
	MpegTSFilter_t filter;
	void * h = NULL;
	char buffer[1024];
	int ret = -1;
	
	h = dmx_init_handle("../mpts.ts");

	memset(&filter, 0, sizeof(MpegTSFilter_t));
	filter.PID = 0x32;
	//filter.Code[0] = (0x82 & 0xF0);
	filter.Code[0] = 0x82;
	filter.Mask[0] = 0xFF;
	filter.pBuf = buffer;
	filter.BufLen = 1024;

	ret = dmx_process_filter(h, &filter);

	dmx_deinit_handle(h);
	#endif

	g_rlota_search_test(120);

	return 0;
}



