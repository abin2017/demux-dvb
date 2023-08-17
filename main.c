
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "demux.h"


int main(int argc, char **argv){
	MpegTSFilter_t filter;
	void * h = NULL;
	char buffer[1024];
	int ret = -1;
	
	h = dmx_init_handle("../mpts.ts");

	memset(&filter, 0, sizeof(MpegTSFilter_t));
	filter.PID = 0x32;
	filter.Code[0] = (0x82 & 0xF0);
	filter.Mask[0] = 0xF0;
	filter.pBuf = buffer;
	filter.BufLen = 1024;

	ret = dmx_process_filter(h, &filter);

	dmx_deinit_handle(h);
	return 0;
}
