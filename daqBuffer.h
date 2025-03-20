#ifndef __DAQBUFFER_H
#define __DAQBUFFER_H


#include <lzo/lzoutil.h>

#include <EvtStructures.h>
#include <daq_device.h>
#include <EventTypes.h>
#include <daqEvent.h>
#include <BufferConstants.h>
#include "md5.h"
#include "rcdaq.h"
#include <arpa/inet.h>


#define CTRL_BEGINRUN        1
#define CTRL_ENDRUN          2
#define CTRL_DATA            3
#define CTRL_CLOSE           4
#define CTRL_SENDFILENAME    5
#define CTRL_ROLLOVER        6

#define CTRL_REMOTESUCCESS 100
#define CTRL_REMOTEFAIL    101



class daqBuffer {

public:

  //** Constructors

  daqBuffer(const int irun = 1, const int length = 16*1024*1024
	    , const int iseq = 1,   md5_state_t *md5state = 0);

  virtual ~daqBuffer();

  //  this creates a new event on the next address
  int nextEvent(const int etype, const int evtseq, const int maxsize);

  int prepare_next( const int iseq, const int irun =-1);

  // subevent adding
  unsigned int addSubevent( daq_device *);

  //  add end-of-buffer
  unsigned int addEoB();

  int setCompression(const int flag);
  int getCompression() const {return wants_compression;} ;

  // now the write routine
  int start_writeout_thread ();
  
  // now the write routine
  unsigned int writeout ();

  // now the "send buffer to a server" routine
  unsigned int sendout ( int fd );
  
  // now the "send monitor data" routine
  unsigned int sendData ( int fd, const int max_length);

  // now the re-sizing of buffer
  int setMaxSize( const int size);

  // getMaxSize is the actually set size in KB 
  // never mind what the mem size is, we can instruct it to use less memory
  int getMaxSize() const ;

  // this is the immutable largest memory size in words 
  int getMemSize() const {return max_size;} ;

  // and the query
  int getBufSeq () const { return bptr->Bufseq; } ;
  unsigned int getLength () const { return bptr->Length; } ;

  unsigned int getBufferNumber() const { return _my_buffernr; } ;
  int getLastEventNumber() const { return _lastEventNumber; };

  int setEventFormat(const int f);
  int getEventFormat() const {return format;};

  // MD5 checksum business
  void setMD5State( md5_state_t *md5state) {_md5state = md5state;};
  md5_state_t * getMD5State() const {return _md5state;};
  void setMD5Enabled(const int x) { if(x) md5_enabled =1; else md5_enabled =0; }
  int  getMD5Enabled() const { return md5_enabled; };

  void setPreviousBuffer( daqBuffer *b) {previousBuffer = b;};
  void setID( const int i) {_my_number=i;};
  int getID() const {return _my_number;};

  void setDirty( const int i) {if (i) _dirty=1; else _dirty = 0;};
  int getDirty() const {return _dirty;};
  int getCompressing() const {return _compressing;};

  int getStatus() const {return _statusword;};
  // bit meaning
  // 0 dirty
  // 1 busy
  // 2 compressing
  // 3 waiting for write
  // 4 writing

  void setVerbosity(const int v) { verbosity = v;};
  int getVerbosity() const {return verbosity;};

  // this allows others to wait for me to finish writing
  int Wait_for_Completion(const int other_buffer) const;
  int Wait_for_free() const;

  static void reset_count();
  static void init_mutex();

  static int getAbsoluteMaxBuffersize()  { return  AbsoluteMaxSize; };
  
protected:

  // now the compress routine
  int compress ();

  // now the write routine
  static void * writeout_thread ( void * me);

  static unsigned long long timespec_subtract(struct timespec *end, struct timespec *start);

  static unsigned int _buffer_count;
  int _my_buffernr;

  int _lastEventNumber;
  
  static pthread_mutex_t M_buffercnt;
  pthread_mutex_t M_eob;  // makes setEoB thread-safe

  typedef struct 
  { 
    unsigned int Length;
    int ID;
    int Bufseq;
    int Runnr;
    int data[1];
  } *buffer_ptr;

  typedef struct
  {
    int fd;
    daqBuffer * me;
  } thread_argument;

  thread_argument _ta;
    
  int _my_number;
  int _busy;
  int _dirty;
  int _compressing;
  int _statusword;
  
  daqBuffer * previousBuffer;
  
  int _broken;
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
  int wants_compression;

  int verbosity;
  
  int currentBufferID;

  pthread_t _writeout_thread_t;


  int md5_enabled;  

  md5_state_t *_md5state;

  static int lzo_initialized;
  lzo_byte *wrkmem;
  lzo_uint  outputarraylength;
  unsigned int *outputarray;

  const static int AbsoluteMaxSize = 2*64*1024*1024;

};

#endif

