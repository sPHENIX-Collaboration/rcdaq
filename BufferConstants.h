#ifndef __BUFFER_CONSTANTS_H
#define __BUFFER_CONSTANTS_H

/* the header length values */ 
#define BUFFERHEADERLENGTH 4
#define EOBLENGTH 4
#define BUFFERSIZE (1024*1024)

#define DAQONCSFORMAT 0
#define DAQPRDFFORMAT 1

#define PRDFBUFFERHEADER 0xffffffc0;
#define ONCSBUFFERHEADER 0xffffc0c0;

#define BUFFERMARKER          0xffffffc0U
#define ONCSBUFFERMARKER      0xffffc0c0U
#define GZBUFFERMARKER        0xfffffafeU
#define LZO1XBUFFERMARKER     0xffffbbfeU
#define LZO1CBUFFERMARKER     0xffffbcfeU
#define ONCSLZO1XBUFFERMARKER 0xffffbbc0U


#define BUFFERBLOCKSIZE 8192



#endif
