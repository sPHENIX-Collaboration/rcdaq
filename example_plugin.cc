#include "example_plugin.h"
#include <daq_device_pluginexample.h>

#include <strings.h>

int example_plugin::create_device(deviceblock *db)
{

  //std::cout << __LINE__ << "  " << __FILE__ << "  " << db->argv0 << std::endl;

  if ( strcasecmp(db->argv0,"device_pluginexample") == 0 ) 
    {
      // we need at least 4 params
      if ( db->npar <4 ) return 1; // indicate wrong params
      
      int eventtype  = atoi ( db->argv1); // event type
      int subid = atoi ( db->argv2); // subevent id

      if  ( db->npar == 4)
	{

	  add_readoutdevice ( new daq_device_pluginexample( eventtype,
                                                     subid,
						     atoi ( db->argv3)));
	  return 0;  // say "we handled this request" 
	}
      else
	{

	  add_readoutdevice ( new daq_device_pluginexample( eventtype,
                                                     subid,
						     atoi ( db->argv3),
						     atoi ( db->argv4)));
	  return 0;  // say "we handled this request" 
	}
    }

  return -1; // say " this is not out device"
}

void example_plugin::identify(std::ostream& os, const int flag) const
{

  if ( flag <=2 )
    {
      os << " - Example Plugin" << std::endl;
    }
  else
    {
      os << " - Example Plugin, provides - " << std::endl;
      os << " -     device_pluginexample (evttype, subid, nwords [,triggerenable]) - readout a silly plugin example device" << std::endl;

    }
      
}

example_plugin _ep;

