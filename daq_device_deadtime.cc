 

#include <iostream>

#include <daq_device_deadtime.h>

using namespace std;

daq_device_deadtime::daq_device_deadtime(const int eventtype
    , const int subeventid
    , const int n_ticks
    , const int n_words
    , const int trigger_enabled)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  number_of_ticks = n_ticks;
  number_of_words = n_words;
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
  
  if ( number_of_words) 
    {
      sevt =  (subevtdata_ptr) adr;
      // set the initial subevent length
      sevt->sub_length =  SEVTHEADERLENGTH;

      // update id's etc
      sevt->sub_id =  m_subeventid;
      sevt->sub_type=4;
      sevt->sub_decoding = ID4EVT;
      sevt->reserved[0] = 0;
      sevt->reserved[1] = 0;

      sevt->sub_padding = number_of_words%2;
      sevt->sub_length += number_of_words + sevt->sub_padding;
      return  sevt->sub_length;
    }

  return  0;
}


void daq_device_deadtime::identify(std::ostream& os) const
{
  
  os  << "Deadtime Device  Event Type: " << m_eventType
      << " n_ticks: " << number_of_ticks << " n_words: " << number_of_words;

    if (th) 
      {
	os << " ** Trigger enabled";
      }

  os << endl;

}

int daq_device_deadtime::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;

  if ( number_of_words)
    {
      return SEVTHEADERLENGTH + number_of_words +2;
    }

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

