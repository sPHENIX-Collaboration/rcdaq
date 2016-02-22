#include <daqBuffer.h>

#include <daqONCSEvent.h>
#include <daqPRDFEvent.h>

#include <unistd.h>

using namespace std;

// the constructor first ----------------
daqBuffer::daqBuffer (const int irun, const int length
		, const int iseq)
{
  int *b = new int [length];
  bptr = ( buffer_ptr ) b;

  data_ptr = &(bptr->data[0]);
  max_length = length;   // in 32bit units
  max_size = max_length;
  bptr->ID = -64;
  bptr->Bufseq = iseq;
  bptr->Runnr = 0;
  current_event = 0;
  current_etype = -1;

  format=DAQONCSFORMAT;

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
  bptr->ID = -64;
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

  //  cout << "maxsize,left: " << max_size << " " << left << endl;

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
  
  // for (int k = 0; k<current_index; k++)
  //  cout << k << " " << bptr->data[k] << endl;

  //  cout << "------------------" << endl;

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
  //  cout << "addind EOB" << endl;
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
  //cout << "writing buffer len = " << getLength() << endl;
  int blockcount = ( getLength() + 8192 -1)/8192;
  int bytecount = blockcount*8192;
  bytes = writen ( fd, (char *) bptr , bytecount );
  return bytes;
}

#define ACKVALUE 101

unsigned int daqBuffer::sendData ( int fd, struct sockaddr_in *si_remote)
{

  fd_set read_flag;
  struct timeval tv;

  FD_ZERO(&read_flag); 
  FD_SET(fd, &read_flag);



  if (!has_end) addEoB();

  socklen_t slen = sizeof(*si_remote);
  ssize_t l;

  char *p = (char *) bptr;
  int sent = 0;

  //  cout << "sending  buffer len = " << getLength()  << endl;

  int total = getLength();
  int ntotal = htonl(total);
  if ( (l = sendto(fd, &ntotal, 4, 0, (sockaddr *) si_remote, slen))==-1)
    {
      perror( "sendto " );
      return 0;
    }

  int quantum;
  int max_sock_length = 48*1024;
  int ackvalue;

  while ( total >0 )
    {
      if ( total > max_sock_length) 
	{
	  quantum = max_sock_length;
	}
      else
	{
	  quantum = total;
	}


      slen = sizeof(*si_remote);
      if ( (l = sendto(fd, p, quantum , 0, (sockaddr *) si_remote, slen))==-1)
	{
	  perror( "sendto " );
	  cout << __FILE__<< " " << __LINE__ << " error sending "  << endl;
	  return 0;
	}

      tv.tv_sec = 0;
      tv.tv_usec= 200000;


      int retval = select(fd+1, &read_flag, NULL, NULL, &tv);
      if ( retval == 0) 
	{
	  cout << __FILE__<< " " << __LINE__ << " timeout waiting for ack "  << endl;
	  return 0;
	}

      slen = sizeof(*si_remote);
      if (recvfrom(fd, &ackvalue, sizeof(ackvalue), 0, (sockaddr *) si_remote, &slen)==-1)
      	    {
      	      cout << __FILE__<< " " << __LINE__ << " error receiving acknowledgement "  << endl;
      	    }

      p+=l;
      total -= l;
      //   cout << "rest: " << total  << " in this call : " << l << " of " << getLength() << endl;
      //    usleep(1 );
    }

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
   if (f) format = DAQPRDFFORMAT;
   else format = DAQONCSFORMAT;

   return 0;
 }

unsigned int daqBuffer::writen (int fd, char *ptr, const unsigned int nbytes)
{

  unsigned int nleft, nwritten;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nwritten = write (fd, ptr, nleft);
      if ( nwritten < 0 ) 
	return nwritten;

      nleft -= nwritten;
      ptr += nwritten;
    }
  return (nbytes-nleft);
}
