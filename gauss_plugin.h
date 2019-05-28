#ifndef __EXAMPLE_PLUGIN_H__
#define __EXAMPLE_PLUGIN_H__

// this is part of the example how to build a plugin.
// It consists of a (silly) daq_device class (which creates a 
// packet of type ID4EVT where channel i has the value i), and
// the actual "plugin" part, a plugabble class called 
// gauss_plugin, which inherits from RCDAQPlugin.

// please note at the very bottom of the .cc file the declaration of 
// a static object of type gauss_plugin which gets 
// instantiated on load of the shared lib and triggers
// the registration. 

// Also note that the plugin class can support more than one 
// daq_device (here we have only one). 

#include <rcdaq_plugin.h>
#include <iostream>

class gauss_plugin : public RCDAQPlugin {

 public:
  int  create_device(deviceblock *db);

  void identify(std::ostream& os = std::cout, const int flag = 0) const;

};


#endif
