#include <daqBuffer.h>

#include <daqONCSEvent.h>
#include <daqPRDFEvent.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <lzo/lzo1x.h>

using namespace std;

int daqBuffer::lzo_initialized = 0;


int readn (int fd, char *ptr, const int nbytes);
int writen (int fd, char *ptr, const int nbytes);


// the constructor first ----------------
daqBuffer::daqBuffer (const int irun, const int length
		      , const int iseq, md5_state_t *md5state)
{
  int *b = new int [length];
  bptr = ( buffer_ptr ) b;

  data_ptr = &(bptr->data[0]);
  max_length = length;   // in 32bit units
  max_size = max_length;
  _broken = 0;
  
  current_event = 0;
  current_etype = -1;

  // preset everything to ONCS format
  format=DAQONCSFORMAT;
  currentBufferID = ONCSBUFFERHEADER;
  _md5state = md5state;
  wants_compression = 0;
  wrkmem = 0;
  outputarraylength = 0;
  outputarray = 0;
  
  prepare_next (iseq, irun);
}


// the destructor... ----------------
daqBuffer::~daqBuffer ()
{
  int *b = (int *)  bptr;
  delete [] b;
  if (outputarray) delete []  outputarray;
  if (wrkmem) delete [] wrkmem;
}



int daqBuffer::prepare_next( const int iseq
			  , const int irun)
{

  //  cout << __FILE__ << " " << __LINE__ << " bptr: " << bptr << endl;
  
  if ( current_event) delete current_event;
  current_event = 0;
  
  // re-initialize the event header length
  bptr->Length =  BUFFERHEADERLENGTH*4;
  bptr->ID = currentBufferID;
  bptr->Bufseq = iseq;
  if (irun>0) bptr->Runnr = irun;

  current_index = 0;
  left = max_size - BUFFERHEADERLENGTH - EOBLENGTH; 
  has_end = 0;

  
  return 0;
}

// ---------------------------------------------------------
int daqBuffer::nextEvent(const int etype, const int evtseq, const int evtsize)
{
  if (current_event) delete current_event;
  current_event = 0;
  current_etype = -1;

  if (evtsize > left-EOBLENGTH) return -1;

  if ( format)
    {
      current_event = new daqPRDFEvent(&(bptr->data[current_index]), evtsize
			     ,bptr->Runnr, etype, evtseq);
      left -= (EVTHEADERLENGTH + 8);
      current_index += EVTHEADERLENGTH+8;
      bptr->Length  += (EVTHEADERLENGTH+8)*4;
    }
  else
    {
      current_event = new daqONCSEvent(&(bptr->data[current_index]), evtsize
			     ,bptr->Runnr, etype, evtseq);
      left -= EVTHEADERLENGTH;
      current_index += EVTHEADERLENGTH;
      bptr->Length  += EVTHEADERLENGTH*4;
    }


  current_etype = etype;

  return 0;
}

// ----------------------------------------------------------
unsigned int daqBuffer::addSubevent( daq_device *dev)
{
  unsigned int len;

  len = current_event->addSubevent(current_etype, dev);

  left -= len;
  current_index += len;
  bptr->Length  += len*4;
  
  return len;
}

// ----------------------------------------------------------
unsigned int daqBuffer::addEoB()
{
  if (has_end) return -1;
  bptr->data[current_index++] = 2;  
  bptr->data[current_index++] = 0;
  bptr->Length  += 2*4;

  has_end = 1;
  if ( current_event) delete current_event;
  current_event = 0;
  return 0;
}

// ----------------------------------------------------------
// int daqBuffer::transfer(dataProtocol * protocol)
// {
//   if (protocol)  
//     return protocol->transfer((char *) bptr, bptr->Length);
//   else
//     return 0;

// }

unsigned int daqBuffer::writeout ( int fd)
{

  if ( _broken) return 0;
  if (!has_end) addEoB();

  unsigned int bytes = 0;;

  if ( ! wants_compression)
    {
      int blockcount = ( getLength() + 8192 -1)/8192;
      int bytecount = blockcount*8192;
      bytes = writen ( fd, (char *) bptr , bytecount );
      if ( _md5state)
	{
	  //cout << __FILE__ << " " << __LINE__ << " updating md5  with " << bytes << " bytes" << endl; 
	  md5_append(_md5state, (const md5_byte_t *)bptr,bytes );
	}
      return bytes;
    }
  else // we want compression
    {
      compress();
      int blockcount = ( outputarray[0] + 8192 -1)/8192;
      int bytecount = blockcount*8192;
      bytes = writen ( fd, (char *) outputarray , bytecount );
      if ( _md5state)
	{
	  //cout << __FILE__ << " " << __LINE__ << " updating md5  with " << bytes << " bytes" << endl; 
	  md5_append(_md5state, (const md5_byte_t *)outputarray,bytes );
	}
      return bytes;
    }
}

#define ACKVALUE 101

unsigned int daqBuffer::sendout ( int fd )
{
  if ( _broken) return 0;

  if (!has_end) addEoB();

  int total = getLength();

  //std::cout << __FILE__ << " " << __LINE__ << " sending  opcode ctrl_data" <<  CTRL_DATA << std::endl ;
  // send "CTRL_DATA" opcode in network byte ordering
  int opcode = htonl(CTRL_DATA);
  int status = writen(fd, (char *) &opcode, sizeof(int));

  //  std::cout << __FILE__ << " " << __LINE__ << " sending  buffer size " <<  total << std::endl ;

  // re-use variable to send the length in network byte ordering 
  opcode = htonl(total);
  status |= writen(fd, (char *) &opcode, sizeof(int));
  
  // now send the actual data
  char *p = (char *) bptr;
  int sent = writen(fd,p,total);

  // wait for acknowledge... we re-use the opcode variable once more
  readn (fd, (char *) &opcode, sizeof(int));
  opcode = ntohl(opcode);
  if ( opcode != CTRL_REMOTESUCCESS) return -1; // signal error
  
  return sent;
}

// this is sending the monitoring data to a client
unsigned int daqBuffer::sendData ( int fd, const int max_length)
{
  if ( _broken) return 0;

  if (!has_end) addEoB();

  int total = getLength();

  if ( total > max_length)
    {
      cout << "Monitoring: data size exceeds limit -- " << total << " limit: " << max_length << endl;  
      total = max_length;
    }

  int ntotal = htonl(total);
  int status = writen(fd, (char *) &ntotal, 4);

  char *p = (char *) bptr;
  int sent = writen(fd,p,total);

  return sent;
}

int daqBuffer::setCompression(const int flag)
{
  if ( !flag)
    {
      wants_compression = 0;
      return 0;
    }
  else
    {
      if ( !  lzo_initialized )
	{
	  if (lzo_init() != LZO_E_OK)
	    {
	      std::cerr << "Could not initialize LZO" << std::endl;
	      _broken = 1;
	    }
	  lzo_initialized = 1;
	}
    
      if ( !wrkmem)
	{
	  wrkmem = (lzo_bytep) lzo_malloc(LZO1X_1_12_MEM_COMPRESS);
	  if (wrkmem)
	    {
	      memset(wrkmem, 0, LZO1X_1_12_MEM_COMPRESS);
	    }
	  else
	    {
	      std::cerr << "Could not allocate LZO memory" << std::endl;
	      _broken = 1;
	      return -1;
	    }
	  outputarraylength = max_length + 8192;
	  outputarray = new unsigned int[outputarraylength];
	}
      wants_compression = 1;
      //cout << " LZO compression enabled" << endl;
      return 0;
    }
}

int daqBuffer::compress ()
{
  if ( _broken) return -1;
  
  lzo_uint outputlength_in_bytes = outputarraylength*4-16;
  lzo_uint in_len = getLength(); 

  lzo1x_1_12_compress( (lzo_byte *) bptr,
			in_len,  
		       (lzo_byte *)&outputarray[4],
			&outputlength_in_bytes,wrkmem);


  outputarray[0] = outputlength_in_bytes +4*BUFFERHEADERLENGTH;
  outputarray[1] = LZO1XBUFFERMARKER;
  outputarray[2] = bptr->Bufseq;
  outputarray[3] = getLength();

  return 0;
}


// ----------------------------------------------------------
int daqBuffer::setMaxSize(const int size)
{
  if (size < 0) return -1;
  if (size == 0) max_size = max_length;
  else
    {
      max_size = (size + 8191)/8192;
      max_size *= 2048;

      if (max_size > max_length)
	{
	  max_size = max_length;
	  return -2;
	}
    }
  return 0;
}

// ----------------------------------------------------------
int daqBuffer::getMaxSize() const 
{
  return max_size*4;
}

int daqBuffer::setEventFormat(const int f) 
 { 
   if (f)
     {
       format = DAQPRDFFORMAT;
       currentBufferID = PRDFBUFFERHEADER;
     }
   else
     {
       format = DAQONCSFORMAT;
       currentBufferID = ONCSBUFFERHEADER;
     }
   
   bptr->ID = currentBufferID;
   return 0;
 }


