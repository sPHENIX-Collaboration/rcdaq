#include <daqBuffer.h>

#include <daqONCSEvent.h>
#include <daqPRDFEvent.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

using namespace std;

int readn (int fd, char *ptr, const int nbytes);
int writen (int fd, char *ptr, const int nbytes);


// the constructor first ----------------
daqBuffer::daqBuffer (const int irun, const int length
		, const int iseq)
{
  int *b = new int [length];
  bptr = ( buffer_ptr ) b;

  data_ptr = &(bptr->data[0]);
  max_length = length;   // in 32bit units
  max_size = max_length;

  current_event = 0;
  current_etype = -1;

  // preset everything to ONCS format
  format=DAQONCSFORMAT;
  currentBufferID = ONCSBUFFERHEADER;

  prepare_next (iseq, irun);
}


// the destructor... ----------------
daqBuffer::~daqBuffer ()
{
  int *b = (int *)  bptr;
  delete [] b;
}



int daqBuffer::prepare_next( const int iseq
			  , const int irun)
{
  
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
      left -= EVTHEADERLENGTH -8 ;
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
int daqBuffer::addSubevent( daq_device *dev)
{
  int len;

  len = current_event->addSubevent(current_etype, dev);

  left -= len;
  current_index += len;
  bptr->Length  += len*4;
  
  return len;
}

// ----------------------------------------------------------
int daqBuffer::addEoB()
{
  if (has_end) return -1;
  bptr->data[current_index++] = 2;  
  bptr->data[current_index++] = 0;
  bptr->Length  += 2*4;

  has_end = 1;
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

  if (!has_end) addEoB();

  unsigned int bytes;
  int blockcount = ( getLength() + 8192 -1)/8192;
  int bytecount = blockcount*8192;
  bytes = writen ( fd, (char *) bptr , bytecount );
  return bytes;
}

#define ACKVALUE 101

unsigned int daqBuffer::sendData ( int fd, const int max_length)
{

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


