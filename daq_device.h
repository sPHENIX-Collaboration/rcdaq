
// device_i.h
#ifndef __DAQ_DEVICE_H__
#define __DAQ_DEVICE_H__

#include <SubevtStructures.h>
#include <SubevtConstants.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "TriggerHandler.h"


int registerTriggerHandler(TriggerHandler *);
int clearTriggerHandler();
int daq_getEventFormat();

int get_uservalue(const int i);

  
class daq_device {

public:

  daq_device();


  virtual ~daq_device();


  virtual void identify(std::ostream& os = std::cout) const = 0;

  // the non-idl virtual functions
  virtual int max_length(const int etype) const =0;

  // functions to do the work
  virtual int init(){return 0;};
  virtual int endrun(){return 0;};
  virtual int rearm( const int etype){return 0;};
  virtual int put_data(const int, int *, const int) =0;

  virtual int subeventid () const 
    {
      return m_subeventid;
    }

  static int formatPacketHdr(int * adr);
  
 protected:
  long m_subeventid;
  long m_eventType;

};



#endif
