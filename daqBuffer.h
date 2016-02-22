#ifndef __DAQBUFFER_H
#define __DAQBUFFER_H

#include <EvtStructures.h>
#include <daq_device.h>
#include <EventTypes.h>
#include <daqEvent.h>
#include <BufferConstants.h>
#include <arpa/inet.h>

#define DAQONCSFORMAT 0
#define DAQPRDFFORMAT 1


class daqBuffer {

public:

  //** Constructors

  daqBuffer(const int irun = 1, const int length = 8*1024*1024+2*8192
	 , const int iseq = 1);

  virtual ~daqBuffer();

  //  this creates a new event on the next address
  int nextEvent(const int etype, const int evtseq, const int maxsize);

  int prepare_next( const int iseq, const int irun =-1);

  // subevent adding
  int addSubevent( daq_device *);

  //  add end-of-buffer
  int addEoB();

  // now the write routine
  unsigned int writeout ( int fd);

  // now the "send monitor data" routine
  unsigned int sendData ( int fd, struct sockaddr_in *si_remote);


  // now the re-sizing of buffer
  int setMaxSize( const int size);

  // and the query
  int getMaxSize() const ;

  // and the query
  int getBufSeq () const { return bptr->Bufseq; } ;
  int getLength () const { return bptr->Length; } ;

  int setEventFormat(const int f);
  int getEventFormat() const {return format;};


protected:

  static unsigned int writen (int fd, char *ptr, const unsigned int nbytes);


  typedef struct 
  { 
    int Length;
    int ID;
    int Bufseq;
    int Runnr;
    int data[1];
  } *buffer_ptr;

  buffer_ptr bptr;
  int *data_ptr;
  int current_index;
  int max_length;
  int max_size;
  int left;
  daqEvent *current_event;
  int current_etype;
  int has_end;
  int format;
};

#endif

