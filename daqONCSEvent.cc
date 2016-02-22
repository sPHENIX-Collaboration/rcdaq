#include <daqONCSEvent.h>

// the constructor first ----------------
daqONCSEvent::daqONCSEvent (int * where, const int length
		, const int irun, const int etype, const int evtseq)
{
  event_base = where;
  evthdr = ( evtdata_ptr ) where;
  evthdr->evt_type = etype;
  max_length = length;
  prepare_next (evtseq, irun);
}


int daqONCSEvent::prepare_next()
{
  return  prepare_next(-1,-1);
}

int daqONCSEvent::prepare_next( const int evtseq, const int irun)
{
  // re-initialize the event header length
  evthdr->evt_length =  EVTHEADERLENGTH;

  // if < 0, just increment the current seq. number
  if ( evtseq < 0 )
    {
      evthdr->evt_sequence++;
    }
  else
    {
      evthdr->evt_sequence = evtseq;
    }

  // if > 0, adjust the run number, else just keep it.
  if ( irun >  0 )
    {
      evthdr->run_number=irun;
    }
  // if > 0, adjust the run number, else just keep it.
  evthdr->date = 0;
  evthdr->time = time(0);
  evthdr->reserved[0] = 0;
  evthdr->reserved[1] = 0;

 
  // reset the current data index, and the leftover counter
  current = 0;
  left=max_length - EVTHEADERLENGTH ;
  return 0;
}


int daqONCSEvent::addSubevent(const int etype, daq_device *dev)
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


int daqONCSEvent::formatHeader(int * where
				      , const int id
				      , const int hf
				      , const int type)
{
  subevtdata_ptr sevt =  (subevtdata_ptr) where;
  sevt->sub_length =  SEVTHEADERLENGTH;

  sevt->sub_id =  id;
  sevt->sub_type=type;
  sevt->sub_decoding = hf;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;
  return 4;
}

