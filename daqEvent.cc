#include <daqEvent.h>

// the constructor first ----------------
daqEvent::daqEvent (int * where, const int length
		, const int irun, const int etype, const int evtseq)
{
  event_base = where;
  evthdr = ( evtdata_ptr ) where;
  evthdr->evt_type = etype;
  max_length = length;
  prepare_next (evtseq, irun);
}

void daqEvent::set_event_type(const int etype)
{
   evthdr->evt_type = etype;
}

int daqEvent::prepare_next()
{
  return  prepare_next(-1,-1);
}

int daqEvent::prepare_next( const int evtseq, const int irun)
{
  // re-initialize the event header length
  evthdr->evt_length =  EVTHEADERLENGTH;

  // if < 0, just increment the current seq. number
  if ( evtseq < 0 )
    evthdr->evt_sequence++;
  else
     evthdr->evt_sequence = evtseq;

  // if > 0, adjust the run number, else just keep it.
  if ( irun >  0 )
    evthdr->run_number=irun;
  
  // reset the current data index, and the leftover counter
  current = 0;
  left=max_length - EVTHEADERLENGTH ;
  return 0;
}
  

int daqEvent::addSubevent(const int etype, daq_device *dev)
{
  int len;

  len = dev->put_data ( etype, &(evthdr->data[current]), left );
  if (left < 0) {
    return 0;
  }
  evthdr->evt_length += len;
  current += len;
  left -= len;
  return len;
}



