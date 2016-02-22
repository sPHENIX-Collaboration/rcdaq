 

#include <iostream>

#include <daq_device_random.h>

using namespace std;

daq_device_random::daq_device_random(const int eventtype
    , const int subeventid
    , const int n_words
    , const int low
    , const int high
    , const int trigger_enabled)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  number_of_words = n_words;
  low_range = low;
  high_range = high;
  rfp = fopen("/dev/urandom","r");
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

daq_device_random::~daq_device_random()
{
  if ( rfp) fclose( rfp);
  if ( th) 
    {
      clearTriggerHandler();
      delete th;
    }
}



// the put_data function

int daq_device_random::put_data(const int etype, int * adr, const int length )
{

  int len = 0;

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  if ( !rfp) return 0;  // if we failed to open the random gen

  if ( daq_getEventFormat() ) // we are writing PRDF
    {

      formatPacketHdr(adr);
      packetdata_ptr sevt =  (packetdata_ptr) adr;
      // update id's etc
      sevt->sub_id =  m_subeventid;
      sevt->sub_type =  4;
      sevt->sub_decoding = 30000+ID4EVT;
      
      int  *d = (int *) &sevt->data;

      int ia;
      int data;

      for ( ia = 0; ia < number_of_words; ia++)
	{
	  int l =  fread ( &data, 4, 1, rfp);
	  if ( l == 1 )
	    {
	      data &= 0x7fffffff; // enforce a posiitive number
	      double x = data;
	      x /= ( 1024. * 1024.);
	      x *= ( high_range - low_range);
	      x /= ( 2. * 1024.);
	      *d++  = x + low_range;
	      len++;
	    }
	}
  
      int padding = len%2;
      sevt->structureinfo += padding;
      len = len + padding;
      sevt->sub_length += len;
      return  sevt->sub_length;
    }

  else
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
      
      unsigned int data;
      
      int  *d = (int *) &sevt->data;
      
      int ia;
      
      for ( ia = 0; ia < number_of_words; ia++)
	{
	  int l =  fread ( &data, 4, 1, rfp);
	  if ( l == 1 )
	    {
	      data &= 0x7fffffff; // enforce a posiitive number
	      double x = data;
	      x /= ( 1024. * 1024.);
	      x *= ( high_range - low_range);
	      x /= ( 2. * 1024.);
	      *d++  = x + low_range;
	      len++;
	    }
	}
      
      sevt->sub_padding = len%2;
      len = len + (len%2);
      sevt->sub_length += len;
      return  sevt->sub_length;
    }
}


void daq_device_random::identify(std::ostream& os) const
{
  
  os  << "Random Device  Event Type: " << m_eventType << " Subevent id: " << m_subeventid 
      << " n_words: " << number_of_words 
      << " range: " << low_range << " - " << high_range;

    if (th) 
      {
	os << " ** Trigger enabled";
      }

  os << endl;

}

int daq_device_random::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (number_of_words + SEVTHEADERLENGTH);
}

int  daq_device_random::init()
{

  return 0;
}

// the rearm() function
int  daq_device_random::rearm(const int etype)
{
  if (etype != m_eventType) return 0;
  return 0;
}

