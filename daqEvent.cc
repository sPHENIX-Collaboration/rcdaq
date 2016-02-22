#include <daqEvent.h>

// the constructor first ----------------
// daqEvent::daqEvent (int * where, const int length
// 		, const int irun, const int etype, const int evtseq)
// {
//   event_base = where;
//   evthdr = ( evtdata_ptr ) where;
//   evthdr->evt_type = etype;
//   max_length = length;
//   prepare_next (evtseq, irun);
// }

void daqEvent::set_event_type(const int etype)
{
   evthdr->evt_type = etype;
}

  



