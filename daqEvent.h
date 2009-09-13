#ifndef __DAQEVENT__
#define __DAQEVENT__


#include <EvtStructures.h>
#include <daq_device.h>

// virtual base class for an "event"

class daqEvent {

public:

  //** Constructors
  daqEvent(int *, const int maxlength
	 , const int irun, const int itype, const int eseq);

  int prepare_next();
  int prepare_next( const int, const int);
  void set_event_type(const int);

  // subevent adding
  int addSubevent(const int etype, daq_device *);

protected:

int *event_base;
int current;
evtdata_ptr evthdr;
int max_length;
int left;

};

#endif
