

#include <iostream>
#include <sstream>
#include <fstream>

#include <daq_device_filenumbers.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

 

using namespace std;

daq_device_filenumbers::daq_device_filenumbers(const int eventtype
				 , const int subeventid 
				 , const char *fn
				 , const int maxlength)

{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  filename = fn;
  _maxlength = maxlength;

}

daq_device_filenumbers::~daq_device_filenumbers()
{
  
}



// the put_data function

int daq_device_filenumbers::put_data(const int etype, int * adr, const int length )
{
  
  if (etype != m_eventType )  // not our id
    {
      return 0;
    }
  
  ifstream sarg (filename.c_str()); 

  if ( !sarg.is_open()) return 0;

  int value;
  string line;

  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;
  
  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=1;
  sevt->sub_decoding = ID4EVT;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;
  
  unsigned int data;
  
  int  *d = (int *) &sevt->data;
  int i = 0;
  int go_on = 1;

  while ( sarg.good() && go_on )
    {
      getline ( sarg, line);
      //      cout << "-- " << line << endl;
      istringstream is ( line);
      if ( is >> d[i]) 
	{
	  i++;
	  if ( i + SEVTHEADERLENGTH + 1 >= length)
	    {
	      cout << __FILE__ << "  " << __LINE__ 
		   <<" too large payload in Packet " <<  m_subeventid 
		   << " current size " << i +SEVTHEADERLENGTH +1  
		   << "  max is " << length << endl;
	      go_on = 0;
	    }
	}
    }

  sevt->sub_padding = i%2;
  sevt->sub_length += (i + sevt->sub_padding);
  return  sevt->sub_length;
}


void daq_device_filenumbers::identify(std::ostream& os) const
{
  
  os  << "File Number Reader  Event Type: " << m_eventType 
      << " Subevent id: " << m_subeventid 
      << " reading from  " << filename << endl;

}

int daq_device_filenumbers::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  _maxlength + SEVTHEADERLENGTH + 1;
}


