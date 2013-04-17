#ifndef __DAQ_DEVICE_DEADTIME__
#define __DAQ_DEVICE_DEADTIME__


#include <daq_device.h>
#include <pulserTriggerHandler.h>
#include <stdio.h>


class daq_device_deadtime: public  daq_device {


public:

  daq_device_deadtime (const int eventtype
    , const int subeventid
    , const int n_ticks = 100
    , const int n_words = 0
    , const int trigger_enabled=0);


  ~daq_device_deadtime();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  int rearm( const int etype);

protected:
  subevtdata_ptr sevt;
  unsigned int number_of_ticks;
  unsigned int number_of_words;
  pulserTriggerHandler *th;

};


#endif
