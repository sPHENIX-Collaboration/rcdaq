#ifndef __DAQPRDFEVENT__
#define __DAQPRDFEVENT__


#include <daqEvent.h>

class daqPRDFEvent : public daqEvent {

public:

  //** Constructors
  daqPRDFEvent(int *, const int maxlength
	 , const int irun, const int itype, const int eseq);

  ~daqPRDFEvent() {};

  int prepare_next();
  int prepare_next( const int, const int);


  // subevent adding
  int addSubevent(const int etype, daq_device *);
  
protected:


};

#endif
