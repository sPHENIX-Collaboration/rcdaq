 

#include <iostream>

#include <daq_device_deadtime.h>

using namespace std;

daq_device_deadtime::daq_device_deadtime(const int eventtype
    , const int subeventid
    , const int n_ticks
    , const int trigger_enabled)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  number_of_ticks = n_ticks;
  if (trigger_enabled) 
    {
      th = new pulserTriggerHandler(0);
      registerTriggerHandler(th);
    }
  else
    {
      th = 0;
    }
}

daq_device_deadtime::~daq_device_deadtime()
{
  if ( th) 
    {
      clearTriggerHandler();
      delete th;
    }
}



// the put_data function

int daq_device_deadtime::put_data(const int etype, int * adr, const int length )
{


  if (etype != m_eventType )  // not our id
    {
      return 0;
    }
  if ( number_of_ticks) usleep ( number_of_ticks);

  return  0;
}


void daq_device_deadtime::identify(std::ostream& os) const
{
  
  os  << "Deadtime Device  Event Type: " << m_eventType
      << " n_ticks: " << number_of_ticks;

    if (th) 
      {
	os << " ** Trigger enabled";
      }

  os << endl;

}

int daq_device_deadtime::max_length(const int etype) const
{
  return 0;
}

int  daq_device_deadtime::init()
{

  return 0;
}

// the rearm() function
int  daq_device_deadtime::rearm(const int etype)
{
  return 0;
}

