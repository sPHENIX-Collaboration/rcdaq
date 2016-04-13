#ifndef __DAQ_DEVICE_FILENUMBERS__
#define __DAQ_DEVICE_FILENUMBERS__


#include <daq_device.h>
#include <stdio.h>
#include <string>

class daq_device_filenumbers: public  daq_device {


public:

  daq_device_filenumbers (const int eventtype,
			  const int subeventid, const char * fn,
			  const int delete_flag = 0,
			  const int maxlength = 256);


  ~daq_device_filenumbers();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);


private:
  subevtdata_ptr sevt;
  unsigned int number_of_words;
  int m_eventType;
  int m_subeventid;
  std::string filename;
  int _maxlength;
  int _delete_flag;
};


#endif
