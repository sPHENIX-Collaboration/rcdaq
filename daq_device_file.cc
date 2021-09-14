 

#include <iostream>

#include <daq_device_file.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

 

using namespace std;

daq_device_file::daq_device_file(const int eventtype
				 , const int subeventid 
				 , const char *fn
				 , const int delete_flag
				 , const int maxlength)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  filename = fn;
  _maxlength = maxlength;
  _delete_flag = 0;
  if ( delete_flag)  _delete_flag = 1;

}

daq_device_file::~daq_device_file()
{
}



// the put_data function

int daq_device_file::put_data(const int etype, int * adr, const int length )
{

  int my_fd;
  int nbytes;

  struct stat buf;

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }
  

  int status = stat (filename.c_str(), &buf); 
  if ( status) 
    {
      return 0;
    }
  else
    {
      nbytes = buf.st_size;
      my_fd = open (filename.c_str(), O_RDONLY | O_LARGEFILE);
      if  ( my_fd < 0) 
	{
	  return 0;
	}
    }
  

 

  if ( daq_getEventFormat() ) // we are writing PRDF
    {


      if ( (nbytes+3)/4 +  6 + 1 >= length)
	{
	  cout << __FILE__ << "  " << __LINE__ 
	       <<" too large payload in Packet " <<  m_subeventid 
	       << "  size " <<  (nbytes+3)/4 + 6 +1  
	       << "  max is " << length << " -- truncated" << endl;
	  nbytes = 4*(length - 6 -2);
	}


      formatPacketHdr(adr);

      packetdata_ptr sevt =  (packetdata_ptr) adr;
  
      // update id's etc
      sevt->sub_id =  m_subeventid;
      sevt->sub_type=1;
      sevt->sub_decoding = 30000 + IDCSTR;
  
      char  *d = (char *) &sevt->data;

      int c = read( my_fd, d, nbytes);

      close (my_fd);

      if ( _delete_flag) unlink (filename.c_str());      

      int padding  = c%8;
      if ( padding) padding = 8-padding;

      sevt->structureinfo += padding;
      sevt->sub_length += (c + padding) /4;

      //      std::cout << __FILE__ << " " << __LINE__ << " c " << c << " pad " << padding << " length  " << sevt->sub_length << std::endl; 

      return  sevt->sub_length;
    }

  else // the good format
    {

      if ( (nbytes+3)/4 +  SEVTHEADERLENGTH + 1 >= length)
	{
	  cout << __FILE__ << "  " << __LINE__ 
	       <<" too large payload in Packet " <<  m_subeventid 
	       << "  size " <<  (nbytes+3)/4 + SEVTHEADERLENGTH +1  
	       << "  max is " << length << " -- truncated" << endl;
	  nbytes = 4*(length - SEVTHEADERLENGTH -2);
	}

      subevtdata_ptr sevt =  (subevtdata_ptr) adr;
      // set the initial subevent length
      sevt->sub_length =  SEVTHEADERLENGTH;
  
      // update id's etc
      sevt->sub_id =  m_subeventid;
      sevt->sub_type=1;
      sevt->sub_decoding = IDCSTR;
      sevt->reserved[0] = 0;
      sevt->reserved[1] = 0;
  
      char  *d = (char *) &sevt->data;

      int c = read( my_fd, d, nbytes);

      close (my_fd);

      if ( _delete_flag) unlink (filename.c_str());      

      sevt->sub_padding = c%8;
      if ( sevt->sub_padding) sevt->sub_padding = 8 - sevt->sub_padding;

      sevt->sub_length += (c + sevt->sub_padding) /4;
      return  sevt->sub_length;
    }

}


void daq_device_file::identify(std::ostream& os) const
{
  
  os  << "File Device  Event Type: " << m_eventType 
      << " Subevent id: " << m_subeventid;
  if ( _delete_flag ) os <<  " reading from and deleting ";
  else os <<  " reading from ";
  os << filename << endl;

}

int daq_device_file::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  _maxlength  + SEVTHEADERLENGTH +1;
}

