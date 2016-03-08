#include <daqPRDFEvent.h>




// the constructor first ----------------
daqPRDFEvent::daqPRDFEvent (int * where, const int length
		, const int irun, const int etype, const int evtseq)
{
  event_base = where;
  evthdr = ( evtdata_ptr ) where;
  evthdr->evt_type = etype;
  max_length = length;
  prepare_next (evtseq, irun);
}


int daqPRDFEvent::addSubevent(const int etype, daq_device *dev)
{
  int len;

  len = dev->put_data ( etype, &(evthdr->data[current]), left );
  if (left < 0) {
    return 0;
  }
  evthdr->evt_length += len;
  evthdr->data[0] += len;
  current += len;
  left -= len;
  return len;
}

int daqPRDFEvent::prepare_next()
{
  return  prepare_next(-1,-1);
}

int daqPRDFEvent::prepare_next( const int evtseq, const int irun)
{
  // re-initialize the event header length
  evthdr->evt_length =  EVTHEADERLENGTH + 8; // plus the frameheader

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
  evthdr->date = time(0);
  evthdr->time = -1;
  evthdr->reserved[0] = 0;
  evthdr->reserved[1] = 0;

  //now the frameheader

  evthdr->data[0] = 8;          //length
  evthdr->data[1] = 0xffffff00; //marker
  evthdr->data[2] = 0x01080000; //version 1 + 8 length
  evthdr->data[3] = 0x00002016; //source id
  evthdr->data[4] = 0x00040000; // frame type 0
  evthdr->data[5] = 0x00000200; // 2 align length
  evthdr->data[6] = 0x55555555; // align 
  evthdr->data[7] = 0x20160000; // align

  // reset the current data index, and the leftover counter
  current = 8;
  left = max_length - EVTHEADERLENGTH - 8 ; // 8 = framelength
  return 0;
}


