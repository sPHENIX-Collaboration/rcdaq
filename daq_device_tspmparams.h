#ifndef __DAQ_DEVICE_TSPMPARAMS__
#define __DAQ_DEVICE_TSPMPARAMS__


#include <daq_device.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <tspmTriggerHandler.h>

class tspm_udp_communication;

class daq_device_tspmparams: public  daq_device {


public:

  daq_device_tspmparams (const int eventtype
			, const int subeventid
			, const char *ipaddr);


  ~daq_device_tspmparams();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  //  int rearm( const int etype);

private:

  subevtdata_ptr sevt;
  char _ip[512];
  int _broken;

  tspm_udp_communication *tspm;


};


#endif
