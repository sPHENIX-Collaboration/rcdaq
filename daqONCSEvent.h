#ifndef __DAQONCSEVENT__
#define __DAQONCSEVENT__


#include <daqEvent.h>

class daqONCSEvent : public daqEvent {

public:

  //** Constructors
  daqONCSEvent(int *, const int maxlength
	 , const int irun, const int itype, const int eseq);

  ~daqONCSEvent() {};

  int prepare_next();
  int prepare_next( const int, const int);

  // subevent adding
  int addSubevent(const int etype, daq_device *);

  static int formatHeader(int * where, const int id, const int hf, const int type);
  
protected:


};

#endif
