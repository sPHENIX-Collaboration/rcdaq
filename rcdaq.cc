
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <stdio.h>
#include <iostream>
#include <iomanip>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>

#include <vector>

#include "getopt.h"

#include "daq_device.h"
#include "daqBuffer.h"
#include "eloghandler.h" 

#include "rcdaq.h"

#define WRITESEM 1
#define WRITEPROTECTSEM 2
#define TRIGGERSEM 3


// those are the "todo" definitions. DAQ can be woken up by a trigger
// and read something, or by a command and change its status.

#define DAQ_TRIGGER 0x01
#define DAQ_COMMAND 0x02
#define DAQ_SPECIAL 0x04

// now there are a few commands which the DAQ process obeys.

#define COMMAND_INIT    1
#define COMMAND_BEGIN   2
#define COMMAND_END     3
#define COMMAND_FINISH  4
#define COMMAND_OPENP   5

// now there are a few actions for the DAQ process
// when DAQ-triggered 
#define DAQ_INIT    1
#define DAQ_READ    2

using namespace std;

static std::string TheFileRule = "rdcaq_%010d_%04d.evt";
static std::string CurrentFilename = " ";
static int daq_open_flag = 0;  //no files written unless asked for
static int file_is_open = 0;
static int current_filesequence = 0;
static int outfile_fd;

static ElogHandler *ElogH =0;


typedef std::vector<daq_device *> devicevector;
typedef devicevector::iterator deviceiterator;

#define MAXEVENTID 32
static int Eventsize[MAXEVENTID];

int Daq_Status;

#define DAQ_RUNNING  0x01
#define DAQ_READING  0x02
#define DAQ_PROTOCOL 0x10

static daqBuffer Buffer1;
static daqBuffer Buffer2;

int Trigger_Todo;
int Command_Todo;
int Origin;

int TheRun = 0;
int Buffer_number;
int Event_number;


daqBuffer  *fillBuffer, *transportBuffer;

pthread_t ThreadId;
pthread_t ThreadEvt;

int *thread_arg;

int end_thread = 0;

pthread_mutex_t M_cout;



devicevector DeviceList;


int semid;

int pid_key = 99;

int NumberWritten = 0; 
unsigned long long BytesInThisRun;
unsigned long long run_volume, max_volume;
unsigned int max_events;


int EvtId = 0;


int max_seconds = 0;
int verbose = 0;
int runnumber=1;
int packetid = 1001;
int max_buffers=0;
int go_on =1;


void sig_handler(int i)
{
  if (verbose) cout << "**interrupt " << endl;
  go_on = 0;
}



int sem_set (const int semnumber, const int value)
{
  union semun 
  {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
  } semset;

  semset.val = value;
  
  return semctl(semid, semnumber, SETVAL, semset);

}


int sem_inc (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;

  operations[0].sem_op = 1;
  operations[0].sem_flg = 0;
  return semop(semid, operations, 1);

}

int sem_dec (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;

  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;
  return semop(semid, operations, 1);

}

int sem_lock (const int semnumber)
{
  struct sembuf operations[2];
  operations[0].sem_num = semnumber;
  operations[0].sem_op = 0;
  operations[0].sem_flg = 0;

  operations[1].sem_num = semnumber;
  operations[1].sem_op = 1;
  operations[1].sem_flg = 0;

  return semop(semid, operations, 2);

}

int sem_wait (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;
  operations[0].sem_op = 0;
  operations[0].sem_flg = 0;

  return semop(semid, operations, 1);

}

int reset_deadtime() {}
int enable_trigger() {}
int disable_trigger() {}



int daq_set_eloghandler( const char *host, const int port, const char *logname)
{

  if ( ElogH) delete ElogH;
  ElogH = new ElogHandler (host, port, logname );
  return 0;
}


//-------------------------------------------------------------

int open_file(const int run_number, int *fd)
{
  
  if ( file_is_open) return -1;


  char d[512];
  sprintf( d, TheFileRule.c_str(),
	   run_number, current_filesequence);


  // test if the file exists, do not overwrite
  int ifd =  open(d, O_WRONLY | O_CREAT | O_EXCL | O_LARGEFILE , 
		  S_IRWXU | S_IROTH | S_IRGRP );
  if (ifd < 0) 
    {
      return -1;
    }

  *fd = ifd;
  CurrentFilename = d;

  file_is_open =1;

  return 0;
}


void *writebuffers ( void * arg)
{
  int i;


  while(1)
    {

      sem_dec( WRITESEM);
      cout << "writing..." << endl;
      unsigned int bytecount = transportBuffer->writeout(outfile_fd);
      NumberWritten++;
      BytesInThisRun += bytecount;

      if (verbose >1)
	{
	  pthread_mutex_lock(&M_cout);
	  cout << __LINE__ << "  " << __FILE__ << " write thread has written  " <<  BytesInThisRun << endl;
	  pthread_mutex_unlock(&M_cout);
	}
      
      
      if ( end_thread) pthread_exit(0);
      
      sem_inc(WRITEPROTECTSEM);
    }
  
}

int switch_buffer()
{

  daqBuffer *spare;
  
  sem_dec ( WRITEPROTECTSEM );
  //switch buffers
  spare = transportBuffer;
  transportBuffer = fillBuffer;
  fillBuffer = spare;


  sem_inc ( WRITESEM );
  return 0;

}
  
int daq_set_filerule(const char *rule)
{
  TheFileRule = rule;
  return 0;
}

std::string& daq_get_filerule()
{
  return TheFileRule;
}

std::string& get_current_filename()
{
  return CurrentFilename;
}

int daq_begin(const int irun, std::ostream& os)
{
  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is already active" << endl;
      return -1;
    }

  if ( ! irun)
    {
      TheRun++;
    }
  else
    {
      TheRun = irun;
    }
  Event_number = 0;

  Command(COMMAND_BEGIN);

  return 0;
}


int Command (const int command)
{
  Command_Todo = command;
  Origin |= DAQ_COMMAND;
  
  sem_inc( TRIGGERSEM);

  return 0;
}

int daq_end(std::ostream& os)
{
  Command(COMMAND_END);
  return 0;
}

int daq_fake_trigger (const int n, const int waitinterval)
{
  int i;
  for ( i = 0; i < n; i++)
    {
      
      Trigger_Todo=DAQ_READ;
      Origin |= DAQ_TRIGGER;
      sem_inc ( TRIGGERSEM );
      cout << "trigger" << endl;
      usleep (200000);

    }
}


void * EventLoop( void *arg)
{
  int size;

  int go_on = 1;
  while (go_on)
    {

      sem_dec ( TRIGGERSEM );
      Origin &= ( DAQ_TRIGGER | DAQ_COMMAND | DAQ_SPECIAL);

      while (Origin)
	{
	  
	  if ( Origin & DAQ_TRIGGER )
	    // yep, a trigger.
	    {
	      if ( Daq_Status & DAQ_RUNNING ) 
		// accept it only when we are running
		{
		  
		  // now see what we have to do.
		  switch (Trigger_Todo)
		    {
		    case DAQ_READ:
		      Daq_Status |= DAQ_READING;
		      readout(DATAEVENT);
		      Daq_Status ^= DAQ_READING;

		      rearm(DATAEVENT);

		      // reset todo, and the DAQ_TRIGGER bit. 
		      Trigger_Todo = 0;
		      Origin ^= DAQ_TRIGGER;
		      
		      reset_deadtime();
		    }
		}
	      else
		// no, we are not running
		{
		  cout << "Run not active" << endl;
		  // reset todo, and the DAQ_TRIGGER bit. 
		  Trigger_Todo = 0;
		  Origin ^= DAQ_TRIGGER;
		}
	    }


	  if ( Origin & DAQ_COMMAND )
	    {
	      //  cout << " command! " <<  ++i << Command_Todo << endl;
							
	      // reset the DAQ_COMMAND bit
	      Origin ^= DAQ_COMMAND;

	      // now look what to do
	      switch (Command_Todo)
		{
		case COMMAND_INIT: 
		  cout << "Init..." << endl;
		  Command_Todo = 0;
		  break;

		case  COMMAND_BEGIN:


		  // if we are already running, return
		  if ( Daq_Status & DAQ_RUNNING ) 
		    {
		      cout << "Run is already active" << endl;
		    }
		  else
		    {
		      // if we have a protocol, say "begin run" 
		      if ( daq_open_flag )
			{
			  int status = open_file ( TheRun, &outfile_fd);
			  if ( !status)
			    {
			      if (ElogH) ElogH->BegrunLog( TheRun,"RCDAQ",  
							   get_current_filename());
			    }
			}
		      //initialize the Buffer and event number
		      Buffer_number = 1;
		      Event_number  = 1;

		      // set the status to "running"
		      Daq_Status |= DAQ_RUNNING;
		      set_eventsizes();
		      // initialize Buffer1 to be the fill buffer
		      fillBuffer      = &Buffer1;
		      transportBuffer = &Buffer2;
		      fillBuffer->prepare_next(Buffer_number,TheRun);
		      BytesInThisRun = 0;

		      device_init();

		      // readout the begin-run event
		      readout(BEGRUNEVENT);

		      //now enable the interrupts and reset the deadtime
		      enable_trigger();
		      reset_deadtime();
		    }
		  Command_Todo = 0;
		  break;

		case COMMAND_END:

		  if ( Daq_Status & DAQ_RUNNING ) 
		    {
		      disable_trigger();
		
		      readout(ENDRUNEVENT);
		      switch_buffer();  // we force a buffer flush
		      sem_dec ( WRITEPROTECTSEM );  // this is to wait for the write
		      sem_inc ( WRITEPROTECTSEM );  // to complete
		
		      if ( file_is_open )
			{
			  if (ElogH) ElogH->EndrunLog( TheRun,"RCDAQ", Event_number); 

			}
		      Daq_Status ^= DAQ_RUNNING;
		      cout << "Run ended" << endl;
		      Event_number = 0;
		      run_volume = 0;    // volume in longwords 
		      BytesInThisRun = 0;    // bytes actually written
		      Buffer_number = 0;
		    }
		  else
		    {
		      cout << "Run is not active" << endl;
		    }
		  Command_Todo = 0;
		  break;
									
		case COMMAND_FINISH:
		  cout << "Finish command!" << endl;
		  Command_Todo = 0;
		  return 0;

		default:
		  cout << "unknown command!" << endl;
		  Command_Todo = 0;
		  break;

		}
	    }
					
	}
    }

}

  

int add_readoutdevice ( daq_device *d)
{
  DeviceList.push_back (d);
  return 0;

}


int device_init()
{

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      (*d_it)->init();
    }

  return 0;
}


int readout(const int etype)
{

  cout << " readout etype = " << etype << endl;

  int len = EVTHEADERLENGTH;

  if (etype < 0 || etype>MAXEVENTID) return 0;

  deviceiterator d_it;


  int status = fillBuffer->nextEvent(etype,Event_number, Eventsize[etype]);
  if (status != 0) 
    {
      switch_buffer();
      status = fillBuffer->nextEvent(etype,Event_number,  Eventsize[etype]);
    }
  Event_number++;


  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      len += fillBuffer->addSubevent ( (*d_it) );
    }

  run_volume += len;

  if ( max_volume > 0 && run_volume >= max_volume) 
    {
      cout << " automatic end after " << max_volume /(256*1024) << " Mb" << endl;
      daq_end(std::cout);
    }

  if ( max_events > 0 && Event_number >= max_events ) 
    {
      cout << " automatic end after " << max_events<< " events" << endl;
      daq_end(std::cout);
    }

  return 0;
}


int rearm(const int etype)
{

  if (etype < 0 || etype>MAXEVENTID) return 0;

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      (*d_it)->rearm(etype);
    }

  return 0;
}

void set_eventsizes()
{
  int i;
  int size;
  deviceiterator d_it;

  for (i = 0; i< MAXEVENTID; i++)
    {
      size = EVTHEADERLENGTH;


      for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
	{
	  size += (*d_it)->max_length(i) ;
	}

      Eventsize[i] = size;
      if (size>EVTHEADERLENGTH)
	cout << "Event id " << i << " size " << size << endl;
    }
}

int daq_open (std::ostream& os)
{

  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }

  daq_open_flag =1;
  return 0;
}

//   if (outfile_fd)
//     {
//       os << "File already open" << endl;
//       return -1;
//     }

//   outfile_fd = open(filename, O_WRONLY | O_CREAT | O_LARGEFILE ,
//                   S_IRWXU | S_IROTH | S_IRGRP );

//   cout << "outfile_fd = "  << outfile_fd << endl;

//   if ( outfile_fd < 0 ) 
//     {
//       perror ("open file");
//       outfile_fd = 0;
//       return -1;
//     }
//   return 0;
// }

int is_open()
{
  return daq_open_flag;
}

int daq_close (std::ostream& os)
{

  if ( Daq_Status & DAQ_RUNNING ) 
    {
      cout << "Run is active" << endl;
      return -1;
    }

  daq_open_flag =0;
  return 0;
}

//   if (!outfile_fd)
//     {
//       cout << "File is not open" << endl;
//       return -1;
//     }

//   close (outfile_fd);
//   outfile_fd = 0;
  
//   return 0;
// }


int daq_list_readlist(std::ostream& os)
{

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      (*d_it)->identify(os);
    }

  return 0;

}

int daq_clear_readlist()
{
  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      delete (*d_it);
    }

  DeviceList.clear();

  return 0;
}


int rcdaq_init( )
{

  int status;

  semid = semget(pid_key, 4, 0666 | IPC_CREAT);

  pthread_mutex_init(&M_cout, 0); 
   
  int i;
  for ( i = 0; i <4 ; i++)
    {
      sem_set(i, 0);
    }

  sem_set ( WRITEPROTECTSEM,1);

  //  signal(SIGKILL, sig_handler);
  //signal(SIGTERM, sig_handler);
  //signal(SIGINT,  sig_handler);
   
  outfile_fd = 0;

  fillBuffer = &Buffer1;
  transportBuffer = &Buffer2;


  status = pthread_create(&ThreadId, NULL, 
			  writebuffers, 
			  (void *) 0);
   
  if (status ) 
    {
      cout << "error in thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "write thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }
   
  status = pthread_create(&ThreadEvt, NULL, 
			  EventLoop, 
			  (void *) 0);
   
  if (status ) 
    {
      cout << "error in thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "event thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }
   

 
}


