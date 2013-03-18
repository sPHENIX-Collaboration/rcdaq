#ifndef __DAQ_DEVICE_COMMAND__
#define __DAQ_DEVICE_COMMAND__


#include <daq_device.h>
#include <stdio.h>
#include <string>

class daq_device_command: public  daq_device {


public:

  daq_device_command (const int eventtype,
		      const int subeventid,
		      const char *command,
		      const int verbose = 0);


  ~daq_device_command();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const { return 0;};

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

private:
  subevtdata_ptr sevt;
  int m_eventType;
  int m_subeventid;
  std::string _command;
  int _verbose;

};


#endif
