 

#include <iostream>

#include <daq_device_file.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

 

using namespace std;

daq_device_file::daq_device_file(const int eventtype
    , const int subeventid , const char *fn)

{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  filename = fn;

  struct stat buf;

  int status = stat (filename.c_str(), &buf); 
  if ( status) 
    {
      my_fd = 0;
      defunct  = 1;
      nbytes = 0;
    }
  else
    {
      nbytes = buf.st_size;

      my_fd = open (filename.c_str(), O_RDONLY | O_LARGEFILE);
      if  ( my_fd < 0) 
	{
	  my_fd = 0;
	  nbytes = 0;
	  defunct = 1;
	}
      defunct = 0;
    }

}

daq_device_file::~daq_device_file()
{
  if ( my_fd > 0) close( my_fd);
}



// the put_data function

int daq_device_file::put_data(const int etype, int * adr, const int length )
{
  
  
  if ( defunct || my_fd <=0 ) return 0;

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }
  
  
  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;
  
  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=1;
  sevt->sub_decoding = IDCSTR;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;
  
  unsigned int data;
  
  char  *d = (char *) &sevt->data;

  off_t x = lseek ( my_fd, 0, SEEK_SET);
  int c = read( my_fd, d, nbytes);

  sevt->sub_padding = c%8;
  sevt->sub_length += (c + sevt->sub_padding) /4;
  return  sevt->sub_length;
}


void daq_device_file::identify(std::ostream& os) const
{
  
  os  << "File Device  Event Type: " << m_eventType 
      << " Subevent id: " << m_subeventid 
      << " reading from  " << filename << endl;

}

int daq_device_file::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (nbytes + (nbytes%8))/4  + SEVTHEADERLENGTH;
}

int  daq_device_file::init()
{

  return 0;
}

// the rearm() function
int  daq_device_file::rearm(const int etype)
{
  return 0;
}

