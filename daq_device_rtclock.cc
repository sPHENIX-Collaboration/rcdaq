 

#include <iostream>
#include <string.h>

#include <daq_device_rtclock.h>

using namespace std;

daq_device_rtclock::daq_device_rtclock(const int eventtype
				     , const int subeventid
				     , const int n_what)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
}

daq_device_rtclock::~daq_device_rtclock()
{
}



// the put_data function

int daq_device_rtclock::put_data(const int etype, int * adr, const int length )
{

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }


  struct timespec clk;


  if ( daq_getEventFormat() ) // we are writing PRDF
    {

      formatPacketHdr(adr);
      packetdata_ptr sevt =  (packetdata_ptr) adr;
      // update id's etc
      sevt->sub_id =  m_subeventid;
      sevt->sub_type =  4;
      sevt->sub_decoding = 30000+9;
      
      int  *d = (int *) &sevt->data;
      int s = clock_gettime(CLOCK_MONOTONIC_RAW, &clk);
      
      if  (s)  // error, set everything to 0
	{
	  for ( int i = 0; i < 6; i++)  *d++ = 0;
	}
      else
	{
	  *d++ = clk.tv_sec;
	  memcpy(d, &clk.tv_nsec, sizeof(clk.tv_nsec));
	  d+=2;
	  *d++ = previous_clk.tv_sec;
	  memcpy(d, &previous_clk.tv_nsec, sizeof(previous_clk.tv_nsec));
	  d+=2;
	}
      previous_clk = clk;

      sevt->sub_length += 6;
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
      sevt->sub_decoding = IDRTCLK;
      sevt->reserved[0] = 0;
      sevt->reserved[1] = 0;
      
      
      int  *d = (int *) &sevt->data;
      int s = clock_gettime(CLOCK_MONOTONIC_RAW, &clk);
      
      if  (s)  // error, set everything to 0
	{
	  for ( int i = 0; i < 6; i++)  *d++ = 0;
	}
      else
	{
	  *d++ = clk.tv_sec;
	  memcpy(d, &clk.tv_nsec, sizeof(clk.tv_nsec));
	  d+=2;
	  *d++ = previous_clk.tv_sec;
	  memcpy(d, &previous_clk.tv_nsec, sizeof(previous_clk.tv_nsec));
	  d+=2;
	}
      previous_clk = clk;

      sevt->sub_padding = 0;
      sevt->sub_length += 6;
      return  sevt->sub_length;
    }
}


void daq_device_rtclock::identify(std::ostream& os) const
{
  
  os  << "Real-time clock Device  Event Type: " << m_eventType << " Subevent id: " << m_subeventid << endl; 

}

int daq_device_rtclock::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (6 + SEVTHEADERLENGTH);
}

int  daq_device_rtclock::init()
{
  int s = clock_gettime(CLOCK_MONOTONIC_RAW, &previous_clk);

  return 0;
}



