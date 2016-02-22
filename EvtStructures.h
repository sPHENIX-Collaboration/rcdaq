#ifndef __EVT_STRUCTURES_H
#define __EVT_STRUCTURES_H

#include "EvtConstants.h"

typedef struct evt_data
 {
   int evt_length;
   int evt_type;
   int evt_sequence;
   int run_number;
   int date;
   int time;
   int reserved[2];
   int data[];
} *evtdata_ptr;


#endif
