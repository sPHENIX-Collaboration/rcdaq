#include <daqBuffer.h>

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

  current_event = new daqEvent(&(bptr->data[current_index]), evtsize
			     ,bptr->Runnr, etype, evtseq);
  left -= EVTHEADERLENGTH;
  current_index += EVTHEADERLENGTH;
  bptr->Length  += EVTHEADERLENGTH*4;

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

unsigned int daqBuffer::sendData ( int fd, struct sockaddr_in *si_remote)
{

  if (!has_end) addEoB();

  socklen_t slen = sizeof(*si_remote);
  ssize_t l;

  char *p = (char *) bptr;
  int sent = 0;

  //  cout << "sending  buffer len = " << getLength()  << endl;
  while ( sent < getLength() )
    {
      if ( (l = sendto(fd, p, 1024, 0, (sockaddr *) si_remote, slen))==-1)
	{
	  perror( "sendto " );
	  return 0;
	}
      p+=l;
      sent += l;
      //      cout << "sent: " << sent  << " in this buffer: " << l << endl;
      usleep(10);
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
