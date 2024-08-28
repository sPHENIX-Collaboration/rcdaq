#include <daqBuffer.h>

#include <daqONCSEvent.h>
#include <daqPRDFEvent.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include <lzo/lzo1.h>

#include <lzo/lzo1b.h>
#include <lzo/lzo1c.h>
#include <lzo/lzo1f.h>
#include <lzo/lzo1x.h>
#include <lzo/lzo1y.h>
#include <lzo/lzo1z.h>

#include <lzo/lzo2a.h>

#include <bzlib.h>

using namespace std;

#define coutfl std::cout << __FILE__<< "  " << __LINE__ << " "
#define cerrfl std::cerr << __FILE__<< "  " << __LINE__ << " "

unsigned int daqBuffer::_buffer_count = 0;
pthread_mutex_t daqBuffer::M_buffercnt  = PTHREAD_MUTEX_INITIALIZER;

int daqBuffer::lzo_initialized = 0;

int UpdateLastWrittenBuffernr (const int n);
int UpdateFileSizes (const unsigned long long size);
int getCurrentOutputFD();
void releaseOutputFD();


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
  verbosity = 0;

  previousBuffer = 0;
  _lastEventNumber = 0;

  _writeout_thread_t  = 0;
  _my_number = 0;
  _dirty = 0;
  _busy = 0;
  _compressing = 0;
  _statusword = 0;

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

  pthread_mutex_init(&M_eob, 0); 
  
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

unsigned long long daqBuffer::timespec_subtract(struct timespec *end, struct timespec *start) 
{
  
  struct timespec result;

  if ((end->tv_nsec - start->tv_nsec) < 0) 
    {
      result.tv_sec = end->tv_sec - start->tv_sec - 1;
      result.tv_nsec = end->tv_nsec - start->tv_nsec + 1000000000L;
    } 
  else 
    {
      result.tv_sec = end->tv_sec - start->tv_sec;
      result.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
  return result.tv_sec* 1000000000L + result.tv_nsec;
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

  _lastEventNumber  = evtseq;

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
  pthread_mutex_lock(&M_eob); 
  if (has_end)
    {
      pthread_mutex_unlock(&M_eob); 
      return -1;
    }
  bptr->data[current_index++] = 2;  
  bptr->data[current_index++] = 0;
  bptr->Length  += 2*4;

  has_end = 1;
  if ( current_event) delete current_event;
  current_event = 0;

  pthread_mutex_unlock(&M_eob); 

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


int daqBuffer::start_writeout_thread ()
{


  int status;
 
  if (_writeout_thread_t) 
    {
      status= pthread_join(_writeout_thread_t, NULL);
      if (status)
	{
	  perror ("start_writeout_thread join");
	  cerrfl << "buffer id " << getID() << endl;
	}
    }
  //_ta.fd =fd;
  _ta.me =this;
  _busy = 1;
  //  _dirty =1;

  // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
  _statusword |= 0x3;  // dirty and busy
  // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

  status = pthread_create(&_writeout_thread_t, NULL, 
			       daqBuffer::writeout_thread, 
			      (void *) &_ta);
  if (status)
    {
      perror ("start_writeout_thread");
      cerrfl << "buffer id " << getID() << endl;
    }

  return status;
}


void * daqBuffer::writeout_thread ( void * x)
{
  thread_argument * ta  = (thread_argument *) x;
  (ta->me)->setDirty(1);
  //register_fd_use(ta->fd,1);
  //int fd = ta->fd;
  (ta->me)->writeout();
  //register_fd_use(ta->fd,0);
  return 0;
}


unsigned int daqBuffer::writeout ()
{

  // if (verbosity) coutfl << " starting compression and writeout for buffer " << getID() << endl;

  if ( _broken) return 0;
  if (!has_end) addEoB();

  
  unsigned int bytes = 0;

  if ( ! wants_compression)
    {
      int blockcount = ( getLength() + 8192 -1)/8192;
      int bytecount = blockcount*8192;


      // if (verbosity)       coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword |= 0x8;
      // if (verbosity)  cout << " to 0x" << _statusword << dec << endl;

      if ( previousBuffer) previousBuffer->Wait_for_Completion(_my_buffernr);

      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword &= ~0x8;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      // if (verbosity) coutfl << "buffer " << getID() << " finished wait on prev id " << previousBuffer->getID() << endl;
      
      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword |= 0x10;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      int fd = getCurrentOutputFD();

      bytes = writen ( fd, (char *) bptr , bytecount );      

      UpdateFileSizes( bytes );
      UpdateLastWrittenBuffernr (getID());

      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword &= ~0x10;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      if ( _md5state && md5_enabled)
	{
	  md5_append(_md5state, (const md5_byte_t *)bptr,bytes );
	}

      releaseOutputFD();

    }

  else // we want compression
    {

      _compressing = 1;
      _statusword |= 0x4;

//      struct timespec t_before, t_after;

      int s = compress();

      _compressing = 0;
      _statusword &= ~0x4;

      int blockcount = ( outputarray[0] + 8192 -1)/8192;
      int bytecount = blockcount*8192;

      // if (verbosity)       coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword |= 0x8;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      if ( previousBuffer) previousBuffer->Wait_for_Completion(_my_buffernr);

      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword &= ~0x8;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword |= 0x10;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      //usleep(1000000);

      int fd = getCurrentOutputFD();

      if (s) // we had a compression error earlier nd write the original instead
	{
	  blockcount = ( getLength() + 8192 -1)/8192;
	  bytecount = blockcount*8192;
	  bytes = writen ( fd, (char *) bptr , bytecount );      
	}
      else
	{
	  bytes = writen ( fd, (char *) outputarray , bytecount );
	}

      UpdateFileSizes( bytes );
      UpdateLastWrittenBuffernr (getID());

      // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
      _statusword &= ~0x10;
      // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

      if ( _md5state && md5_enabled)
	{
	  if (s)
	    {
	      md5_append(_md5state, (const md5_byte_t *)bptr,bytes );
	    }
	  else
	    {
	      md5_append(_md5state, (const md5_byte_t *)outputarray,bytes );
	    }
	}
      releaseOutputFD();

    }
  // if (verbosity) coutfl << "Finishing write for buffer " << getID() << endl;
 
  _busy = 0;
  _dirty = 0;

  // if (verbosity) coutfl << "status change in buffer " << getID() << " from 0x" << hex <<_statusword;
  _statusword = 0;
  // if (verbosity) cout << " to 0x" << _statusword << dec << endl;

  // if (verbosity) coutfl << "buffer " << getID() << " finished write, status= " << getStatus() << endl;

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
  wants_compression = flag;

  // 0 no compression
  // 1 lzo1c
  // 2 lzo2a
  // 3 bz2

  if ( wants_compression < 0 ) wants_compression = 0; 
  if ( wants_compression > 3 ) wants_compression = 3; 

  //coutfl << " compression level " << wants_compression << " set fpor buffer " << getID() << " " << endl;
  
  if ( flag == 0)
    {
      return 0;
    }
  // we initialize the work mem if it's LZO
  else if ( wants_compression == 1 || wants_compression == 2) 
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
      //cout << " LZO compression enabled" << endl;
    }
  else // bz2
    {
      outputarraylength = max_length + 10*8192;
      outputarray = new unsigned int[outputarraylength];
    }
  return 0;
}

int daqBuffer::compress ()
{
  if ( _broken) return -1;

  int result;

  if ( wants_compression == 1 || wants_compression == 2) // LZO-type
    {
      lzo_uint outputlength_in_bytes = outputarraylength*4-16;
      lzo_uint in_len = getLength(); 
      if ( wants_compression == 1 )
	{
	  //LZO_EXTERN(int)
	  result = lzo1c_compress( (lzo_byte *) bptr,
			  in_len,  
			  (lzo_byte *)&outputarray[4],
			  &outputlength_in_bytes,wrkmem, 9);
	  outputarray[0] = outputlength_in_bytes +4*BUFFERHEADERLENGTH;
	  outputarray[1] = LZO1CBUFFERMARKER;
	}
      else
	{
	  result = lzo2a_999_compress( (lzo_byte *) bptr,
			      in_len,  
			      (lzo_byte *)&outputarray[4],
			      &outputlength_in_bytes,wrkmem);
	  outputarray[0] = outputlength_in_bytes +4*BUFFERHEADERLENGTH;
	  outputarray[1] = LZO2ABUFFERMARKER;
	}
    }
  else  // bz2
    {
      unsigned int outputlength_in_bytes = outputarraylength*4-16;
      result = BZ2_bzBuffToBuffCompress((char *) &outputarray[4], &outputlength_in_bytes,
					    (char *) bptr, getLength(),
					    1, // Compression level: 1 (fastest) to 9 (best compression)
					    0, // Verbosity: 0 is quiet
					    0); // Work factor: 0 is default
      outputarray[0] = outputlength_in_bytes +4*BUFFERHEADERLENGTH;
      outputarray[1] = BZ2BUFFERMARKER;

    }

  //coutfl << "orig, new size for buffer " << getID() << " " << getLength() << " " << outputlength_in_bytes << endl;
  
  outputarray[2] = bptr->Bufseq;
  outputarray[3] = getLength();

  if (result) cerrfl << "Compression failed with error code: " << result << std::endl;

  return result;
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
   int guard = 20*10000;
   
   if ( other_buffernumber < _my_buffernr ) return 0;
   while ( _busy  )
     {
       usleep(100);
       if ( --guard <=0)
	 {
	   coutfl << "breaking wait completion lock, I am " << _my_buffernr  << " waited on by " << other_buffernumber << endl;
	   return 0;
	 }
     }
   return 0;
 }

int daqBuffer::Wait_for_free() const
 { 
   int guard = 20*10000;
   //coutfl << "Buffer " << _my_number  << " busy_state: " << _busy << endl;
   while ( _busy  )
     {
       usleep(100);
       if ( --guard <=0)
	 {
	   coutfl << "breaking wait for free lock, I am " << _my_buffernr  << endl;
	   return 0;
	 }
     }
   
   return 0;
 }


