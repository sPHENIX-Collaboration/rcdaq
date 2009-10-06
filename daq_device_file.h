#ifndef __DAQ_DEVICE_FILE__
#define __DAQ_DEVICE_FILE__


#include <daq_device.h>
#include <stdio.h>
#include <string>

class daq_device_file: public  daq_device {


public:

  daq_device_file (const int eventtype,
		   const int subeventid, const char * fn);


  ~daq_device_file();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  int rearm( const int etype);

private:
  subevtdata_ptr sevt;
  unsigned int number_of_words;
  std::string filename;

  int my_fd;
  off_t  nbytes;
  int defunct;
};


#endif
