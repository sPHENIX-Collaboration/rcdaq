#ifndef __SUBEVTSTRUCTURES_H__
#define __SUBEVTSTRUCTURES_H__

#include "SubevtConstants.h"

typedef struct subevt_data
 {
   int   sub_length;
   short sub_id;
   short sub_type;
   short sub_decoding;
   short sub_padding;
   short reserved[2];
   int   data;
 } *subevtdata_ptr;

typedef struct packet_data
 {
   int   sub_length;

   // char hdrversion;
   // char hdrlength;
   // short status;
   int hdrinfo;

   int  sub_id;

   short debug_length;
   short error_length;

   /* char structure; */
   /* char numDescWds; */
   /* char endianism; */
   /* char sub_padding; */
   int structureinfo;

   short sub_decoding;
   short sub_type;

   int   data;
 } *packetdata_ptr;

#endif /* __SUBEVTSTRUCTURES_H__ */
