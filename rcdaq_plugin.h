#ifndef __RCDAQ_PLUGIN_H__
#define __RCDAQ_PLUGIN_H__


//#include <rcdaq.h>
#include <rcdaq_rpc.h>
#include <iostream>


class RCDAQPlugin;
class daq_device;

int add_readoutdevice( daq_device *d);

void plugin_register(RCDAQPlugin * );
void plugin_unregister(RCDAQPlugin *);

/** This is the pure virtual parent class for any plugin

Upon loading, it registers itself with rcdaq

*/


class RCDAQPlugin
{

 public:
  RCDAQPlugin()
    {
      plugin_register(this);
    }
  
  virtual ~RCDAQPlugin()
    {
      plugin_unregister(this);
    }

  // this returns
  //  0 for all ok
  // -1 for "I don't know this device"
  //  1 for "I know this device but the parameters are wrong"

  virtual int  create_device(deviceblock *db) = 0;

  virtual void identify(std::ostream& os = std::cout) const =0; 

  
 protected:
  
  
};


#endif 
