#ifndef __DAQ_DEVICE_RTCLOCK__
#define __DAQ_DEVICE_RTCLOCK__


#include <daq_device.h>
#include <pulserTriggerHandler.h>
#include <stdio.h>
#include <time.h>

class daq_device_rtclock: public  daq_device {


public:

  daq_device_rtclock (const int eventtype
		      , const int subeventid
		      , const int what = 0);


  ~daq_device_rtclock();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  //  int rearm( const int etype);

protected:
  subevtdata_ptr sevt;
  struct timespec previous_clk;

};


#endif
