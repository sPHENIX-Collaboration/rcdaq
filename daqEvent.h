#ifndef __DAQEVENT__
#define __DAQEVENT__



#include <EvtStructures.h>
#include <daq_device.h>

// virtual base class for an "event"

class daqEvent {

public:

  virtual ~daqEvent(){};

  //** Constructors
  //  daqEvent(int *, const int maxlength
  //	 , const int irun, const int itype, const int eseq);

  virtual int prepare_next() = 0;
  virtual int prepare_next( const int, const int) = 0;
  virtual void set_event_type(const int);

  // subevent adding
  virtual int addSubevent(const int etype, daq_device *) =0;

protected:

int *event_base;
int current;
evtdata_ptr evthdr;
int max_length;
int left;

};

#endif
