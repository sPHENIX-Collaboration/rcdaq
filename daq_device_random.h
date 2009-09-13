#ifndef __DAQ_DEVICE_RANDOM__
#define __DAQ_DEVICE_RANDOM__


#include <daq_device.h>
#include <stdio.h>


class daq_device_random: public  daq_device {


public:

  daq_device_random (const int eventtype
    , const int subeventid
    , const int n_words=32
    , const int low=0
    , const int high=2047);


  ~daq_device_random();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  int rearm( const int etype);

private:
  subevtdata_ptr sevt;
  unsigned int number_of_words;
  int low_range;
  int high_range;
  FILE *rfp;

};


#endif
