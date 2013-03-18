 

#include <iostream>

#include <daq_device_command.h>

#include <unistd.h>
#include <stdlib.h>

using namespace std;

daq_device_command::daq_device_command(const int eventtype
				       , const int subeventid 
				       , const char *command
				       , const int verbose)

{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  _command = command;
  _verbose = verbose;


}

daq_device_command::~daq_device_command()
{
}



// the put_data function

int daq_device_command::put_data(const int etype, int * adr, const int length )
{
  

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  system ( _command.c_str() );

  return 0;

}


void daq_device_command::identify(std::ostream& os) const
{
  
  os  << "Command Device  Event Type: " << m_eventType 
      << " executing " << _command << endl;

}


