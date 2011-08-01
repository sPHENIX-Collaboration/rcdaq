#ifndef __DAQ_DEVICE_TSPMPROTO__
#define __DAQ_DEVICE_TSPMPROTO__


#include <daq_device.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class daq_device_tspmproto: public  daq_device {


public:

  daq_device_tspmproto (const int eventtype
			, const int subeventid
			, const char *ipaddr);


  ~daq_device_tspmproto();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  int rearm( const int etype);

private:
  int start_tspm();

  int set_value( const short reg, const int value, int s, struct sockaddr_in *server);

  subevtdata_ptr sevt;
  char _ip[512];
  int _s;
  struct sockaddr_in _si_me;
  int _broken;
};


#endif
