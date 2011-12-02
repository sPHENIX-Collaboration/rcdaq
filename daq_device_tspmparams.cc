
#include <iostream>

#include <daq_device_tspmparams.h>
#include <tspm_udp_communication.h>

#include <string.h>

using namespace std;

daq_device_tspmparams::daq_device_tspmparams(const int eventtype
					   , const int subeventid
					   , const char *ipaddr)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  strcpy ( _ip, ipaddr);

  _broken = 0;

  tspm = new tspm_udp_communication (_ip);
  tspm->init();



}

daq_device_tspmparams::~daq_device_tspmparams()
{
  delete tspm;
}



// the put_data function

int daq_device_tspmparams::put_data(const int etype, int * adr, const int length )
{

  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  int len = 0;

  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;

  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=4;
  sevt->sub_decoding = 71;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;

  if ( length < max_length(etype) ) 
    {
      cout << __LINE__ << "  " << __FILE__ << " length " << length <<endl;
      return 0;
    }

  int i;

  unsigned int *d = (unsigned int *) &sevt->data;
  for ( int k=0; k < 41; k++)
    {    
      i = tspm->get_value( k, d++);
      if (i) 
	{
	  _broken = 1;
	  cout << __LINE__ << "  " << __FILE__ << " udp read status = " << i  <<endl;
	  return 0;
	}
      len++;
      
    }

  sevt->sub_padding = len%2;
  len = len + (len%2);
  sevt->sub_length += len;
  //  cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;

  return  sevt->sub_length;
}


void daq_device_tspmparams::identify(std::ostream& os) const
{
  
  os  << "Tspmparams Device  Event Type: " << m_eventType << " Subevent id: " << m_subeventid 
      << " IP: " << _ip;
  if ( _broken) os << " ** not functional ** ";
  os << endl;

}

int daq_device_tspmparams::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (42 + SEVTHEADERLENGTH);
}

int  daq_device_tspmparams::init()
{

  return 0;

}

// the rearm() function
//int  daq_device_tspmparams::rearm(const int etype)
//{
//  if (etype != m_eventType) return 0;
//  return 0;
//}

