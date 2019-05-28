#include "daq_device_gauss.h"

#include <strings.h>
#include "simpleRandom.h"
 
using namespace std;

daq_device_gauss::daq_device_gauss(const int eventtype
				   , const int subeventid
				   , const int trigger_enabled)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  _mean = 0;
  _sigma = 1.;

  R = new simpleRandom (876565);
  
  if (trigger_enabled) 
    {
      th = new pulserTriggerHandler();
      registerTriggerHandler(th);
    }
  else
    {
      th = 0;
    }
}

daq_device_gauss::~daq_device_gauss()
{
  delete R;

  clearTriggerHandler();
  delete th;
}



// the put_data function

int daq_device_gauss::put_data(const int etype, int * adr, const int length )
{

  int len = 0;

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }


  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;

  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=4;
  sevt->sub_decoding = ID4EVT;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;

  unsigned int data;

  int  *d = (int *) &sevt->data;

  int ia;

  float scale=10;
  for ( ia = 0; ia < 4; ia++)
    {
	 *d++  = R->gauss(_mean ,  scale * _sigma );
	 scale *=10;
	 len++;
    }
  
  
  sevt->sub_padding = len%2;
  len = len + (len%2);
  sevt->sub_length += len;
  return  sevt->sub_length;
}


void daq_device_gauss::identify(std::ostream& os) const
{
  
  os  << "Gaussian Distribution Device  Event Type: " << m_eventType << " Subevent id: " << m_subeventid;

    if (th) 
      {
	os << " ** Trigger enabled";
      }

  os << endl;

}

int daq_device_gauss::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (4 + SEVTHEADERLENGTH);
}

int  daq_device_gauss::init()
{

  return 0;
}

// the rearm() function
int  daq_device_gauss::rearm(const int etype)
{
  if (etype != m_eventType) return 0;
  return 0;
}


