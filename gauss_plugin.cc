#include "gauss_plugin.h"
#include <daq_device_gauss.h>

#include <strings.h>

int gauss_plugin::create_device(deviceblock *db)
{

  //std::cout << __LINE__ << "  " << __FILE__ << "  " << db->argv0 << std::endl;

  if ( strcasecmp(db->argv0,"device_gauss") == 0 ) 
    {
      // we need at least 3 params
      if ( db->npar <3 ) return 1; // indicate wrong params
      
      int eventtype  = atoi ( db->argv1); // event type
      int subid = atoi ( db->argv2); // subevent id

      if  ( db->npar == 3)
	{

	  add_readoutdevice ( new daq_device_gauss( eventtype,
						    subid));
	  return 0;  // say "we handled this request" 
	}
      else
	{

	  add_readoutdevice ( new daq_device_gauss( eventtype,
                                                     subid,
						    atoi ( db->argv3) ));
	  return 0;  // say "we handled this request" 
	}
    }

  return -1; // say " this is not out device"
}

void gauss_plugin::identify(std::ostream& os, const int flag) const
{

  if ( flag <=2 )
    {
      os << " - Gaussian distribution Plugin" << std::endl;
    }
  else
    {
      os << " - Gaussian distribution Plugin, provides - " << std::endl;
      os << " -     device_gauss (evttype, subid [,triggerenable]) - fake device making gaussian distributed data" << std::endl;
      os << "                                                        used for the examples in the pmonitor manual" << std::endl;

    }
      
}

gauss_plugin _gp;

