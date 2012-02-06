#ifndef __DAQ_DEVICE_PLUGINEXAMPLE__
#define __DAQ_DEVICE_PLUGINEXAMPLE__



#include <daq_device.h>
#include <pulserTriggerHandler.h>
#include <iostream>



// this is part of the example how to build a plugin.
// It consists of a (silly) daq_device class (which creates a 
// packet of type ID4EVT where channel i has the value i), and
// the actual "plugin" part, a plugabble class called 
// example_plugin, which inherits from RCDAQPlugin.

// Also note that the plugin class can support more than one 
// daq_device (here we have only one). 

class daq_device_pluginexample : public  daq_device {

public:

  daq_device_pluginexample (const int eventtype
    , const int subeventid
    , const int n_words=32
    , const int trigger_enabled=0);


  ~daq_device_pluginexample();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();

  int rearm( const int etype);

protected:
  subevtdata_ptr sevt;
  unsigned int number_of_words;
  pulserTriggerHandler *th;

};


#endif
