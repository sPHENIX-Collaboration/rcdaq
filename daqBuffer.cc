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
#include <lzo/lzo1c.h>

//#include <bzlib.h>

using namespace std;

#define coutfl std::cout << __FILE__<< "  " << __LINE__ << " "
#define cerrfl std::cerr << __FILE__<< "  " << __LINE__ << " "

unsigned int daqBuffer::_buffer_count = 0;
pthread_mutex_t daqBuffer::M_buffercnt  = PTHREAD_MUTEX_INITIALIZER;

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

  previousBuffer = 0;
  _writeout_thread_t  = 0;
  _my_number = 0;
  _dirty = 0;
  _busy = 0;
  _compressing = 0;
  current_event = 0;
  current_etype = -1;

  // preset everything to ONCS format
  format=DAQONCSFORMAT;
  currentBufferID = ONCSBUFFERHEADER;
  _md5state = md5state;
  wants_compression = 0;
  md5_enabled = 1;
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

void daqBuffer::reset_count()
{
  _buffer_count = 0;
}

void daqBuffer::init_mutex()
{
  pthread_mutex_init(&M_buffercnt, 0); 
}

int daqBuffer::prepare_next( const int iseq
			  , const int irun)
{

  pthread_mutex_lock(&M_buffercnt);
  _buffer_count++;
  _my_buffernr = _buffer_count;
  pthread_mutex_unlock(&M_buffercnt);

  
  
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


int daqBuffer::start_writeout_thread (int fd)
{
  _ta.fd =fd;
  _ta.me =this;
  _busy = 1;
  _dirty =1;
  int status = pthread_create(&_writeout_thread_t, NULL, 
			       daqBuffer::writeout_thread, 
			      (void *) &_ta);
  return status;
}


void * daqBuffer::writeout_thread ( void * x)
{
  thread_argument * ta  = (thread_argument *) x;
  int fd = ta->fd;
  (ta->me)->writeout(fd);
  return 0;
}


unsigned int daqBuffer::writeout ( int fd)
{

  //coutfl << " starting writeout for " << getID() << endl;

  if ( _broken) return 0;
  if (!has_end) addEoB();

  
  unsigned int bytes = 0;

  if ( ! wants_compression)
    {
      int blockcount = ( getLength() + 8192 -1)/8192;
      int bytecount = blockcount*8192;

      if ( previousBuffer->getBufferNumber() < getBufferNumber()) // ok, we need to wait for the other buffer b/c it nees to come first
	{
	  //coutfl << "buffer " << getID() << " with buffernr " << getBufferNumber() 
	  //	 << " calling wait on prev id " << previousBuffer->getID() << " with buffernr " << previousBuffer->getBufferNumber() << endl;
	  if ( previousBuffer) previousBuffer->Wait_for_Completion(_my_buffernr);
	  //coutfl << "buffer " << getID() << " finished wait on prev id " << previousBuffer->getID() << endl;
	}
      
      bytes = writen ( fd, (char *) bptr , bytecount );
      if ( _md5state && md5_enabled)
	{
	  //coutfl << "updating md5  with " << bytes << " bytes for buffer " << getID() << endl; 
	  md5_append(_md5state, (const md5_byte_t *)bptr,bytes );
	}
    }
  else // we want compression
    {

      _compressing = 1;
      compress();
      _compressing = 0;

      int blockcount = ( outputarray[0] + 8192 -1)/8192;
      int bytecount = blockcount*8192;

      if ( previousBuffer) previousBuffer->Wait_for_Completion(_my_buffernr);
      
      bytes = writen ( fd, (char *) outputarray , bytecount );
      if ( _md5state && md5_enabled)
	{
	  //cout << __FILE__ << " " << __LINE__ << " updating md5  with " << bytes << " bytes" << endl; 
	  md5_append(_md5state, (const md5_byte_t *)outputarray,bytes );
	}
    }
  //coutfl << "Finishing write for buffer " << getID() << endl;
 
  _busy = 0;
  _dirty = 0;
  return bytes;
}

#define ACKVALUE 101

unsigned int daqBuffer::sendout ( int fd )
{
  if ( _broken) return 0;

  if (!has_end) addEoB();

  int total = getLength();

  if ( previousBuffer) previousBuffer->Wait_for_Completion(_my_buffernr);

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
	  //	  wrkmem = (lzo_bytep) lzo_malloc(LZO1X_1_12_MEM_COMPRESS);
	  int memsize=LZO1X_999_MEM_COMPRESS;
	  //int memsize=LZO1X_1_15_MEM_COMPRESS;
	  wrkmem = (lzo_bytep) lzo_malloc(memsize);
	  if (wrkmem)
	    {
	      // memset(wrkmem, 0, LZO1X_1_12_MEM_COMPRESS);
	      memset(wrkmem, 0, memsize);
	    }
	  else
	    {
	      std::cerr << "Could not allocate LZO memory" << std::endl;
	      _broken = 1;
	      return -1;
	    }
	  outputarraylength = max_length + 10*8192;
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

  // lzo1x_1_15_compress( (lzo_byte *) bptr,
  // 			in_len,  
  // 		       (lzo_byte *)&outputarray[4],
  // 			&outputlength_in_bytes,wrkmem);



  //  LZO_EXTERN(int)
    lzo1c_compress( (lzo_byte *) bptr,
		    in_len,  
		    (lzo_byte *)&outputarray[4],
    		    &outputlength_in_bytes,wrkmem, 9);
    

  // lzo1x_999_compress( (lzo_byte *) bptr,
  // 			in_len,  
  // 		       (lzo_byte *)&outputarray[4],
  // 			&outputlength_in_bytes,wrkmem);

  // unsigned int outputlength_in_bytes = outputarraylength*4-16;
  // int result = BZ2_bzBuffToBuffCompress((char *) &outputarray[4], &outputlength_in_bytes,
  // 					(char *) bptr, getLength(),
  //                                         9, // Compression level: 1 (fastest) to 9 (best compression)
  //                                         0, // Verbosity: 0 is quiet
  //                                         0); // Work factor: 0 is default

  // if (result != BZ_OK) {
  //   std::cerr << "Compression failed with error code: " << result << std::endl;
  //   return 1;
  // }

  //coutfl << "orig, new size for buffer " << getID() << " " << getLength() << " " << outputlength_in_bytes << endl;
  
  outputarray[0] = outputlength_in_bytes +4*BUFFERHEADERLENGTH;
  outputarray[1] = LZO1CBUFFERMARKER;
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

int daqBuffer::Wait_for_Completion( const int other_buffernumber) const
 { 

   if ( other_buffernumber < _my_buffernr ) return 0;
   while ( _busy  ) usleep(100);

   return 0;
 }

int daqBuffer::Wait_for_free() const
 { 
   //coutfl << "Buffer " << _my_number  << " busy_state: " << _busy << endl;
   while ( _busy ) usleep(100);

   return 0;
 }


