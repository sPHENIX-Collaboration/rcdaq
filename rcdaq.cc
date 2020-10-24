
//#define WRITEPRDF

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
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
#include <sys/socket.h>
#include <netdb.h>

#include <vector>
#include <map>
#include <queue>



#include "getopt.h"

#include "daq_device.h"
#include "daqBuffer.h"
#include "eloghandler.h" 
#include "TriggerHandler.h" 

#include "rcdaq.h"
#include "rcdaq_rpc.h"

pthread_mutex_t WriteSem;
pthread_mutex_t WriteProtectSem;

pthread_mutex_t MonitoringRequestSem;
pthread_mutex_t SendSem;
pthread_mutex_t SendProtectSem;

pthread_mutex_t FdManagementSem;


pthread_mutex_t TriggerSem;
pthread_mutex_t TriggerDone;


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

static std::map<string,string> RunTypes;

static std::string TheFileRule = "rcdaq-%08d-%04d.evt";
static std::string CurrentFilename = "";
static std::string PreviousFilename = "";

static std::string TheRunnumberfile = " ";
static int RunnumberfileIsSet = 0;

static std::string MyName = ""; 

static int daq_open_flag = 0;  //no files written unless asked for
static int file_is_open = 0;
static int current_filesequence = 0;
static int outfile_fd;

static int CurrentEventType = 0;

static ElogHandler *ElogH =0;
static TriggerHandler  *TriggerH =0;


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

int TheRun = 0;
time_t StartTime = 0;
int Buffer_number;
int Event_number;

int TriggerControl = 0;

int ThePort=8899;

daqBuffer  *fillBuffer, *transportBuffer;

pthread_t ThreadId;
pthread_t ThreadMon;
pthread_t ThreadEvt;
pthread_t ThreadTrigger;
pthread_t ThreadWeb = 0;

int *thread_arg;

int end_thread = 0;

pthread_mutex_t M_cout;


struct sockaddr_in si_mine;

#define MONITORINGPORT 9930

void * daq_triggerloop (void * arg);
void * mg_server (void *arg);
int mg_end();


devicevector DeviceList;



int NumberWritten = 0; 
unsigned long long BytesInThisRun = 0;
unsigned long long run_volume, max_volume;
unsigned int max_events;


time_t old_time; // to measure MB/s
double old_volume = 0;
double mb_per_second = 0; 

int EvtId = 0;


int max_seconds = 0;
int verbose = 0;
int runnumber=1;
int packetid = 1001;
int max_buffers=0;
int go_on =1;

int adaptivebuffering = 15;
int last_bufferwritetime  = 0;


int registerTriggerHandler ( TriggerHandler *th)
{
  // we already have one
  if ( TriggerH ) return -1;
  TriggerH = th;
  return 0;
}

int clearTriggerHandler ()
{
  TriggerH = 0;
  return 0;
}


void sig_handler(int i)
{
  if (verbose)
    {
        pthread_mutex_lock(&M_cout);
	cout << "**interrupt " << endl;
	pthread_mutex_unlock(&M_cout);
    }

  go_on = 0;
}


int reset_deadtime() 
{
  if ( TriggerH) TriggerH->rearm();
  pthread_mutex_unlock ( &TriggerDone );
  return 0;
}

int enable_trigger() 
{

  TriggerControl=1;
  if ( TriggerH) TriggerH->enable();

  int status = pthread_create(&ThreadTrigger, NULL, 
  			  daq_triggerloop, 
  			  (void *) 0);

  if (status ) 
    {
      pthread_mutex_lock(&M_cout);
      cout << "error in thread create " << status << endl;
      pthread_mutex_unlock(&M_cout);

      ThreadTrigger = 0;
    }
  else
    {
      pthread_mutex_lock(&M_cout);
      cout << "trigger loop created for run " << TheRun<< endl;
      pthread_mutex_unlock(&M_cout);
      if ( TriggerH) TriggerH->rearm();

    }

  return 0;
  
}

int disable_trigger() 
{
  TriggerControl=0;  // this makes the trigger process terminate
  if ( TriggerH) TriggerH->disable();
  

  sleep (1);

  // pthread_cancel(ThreadTrigger);
  // cout << __FILE__ << " " << __LINE__ << " waiting for trigger loop to exit " << endl;
  // pthread_join(ThreadTrigger, NULL);
  // cout << __FILE__ << " " << __LINE__ << " done " << endl;


  return 0;
}


int daq_setmaxevents (const int n, std::ostream& os)
{

  max_events =n;
  return 0;

}

int daq_setmaxvolume (const int n_mb, std::ostream& os)
{
  unsigned long long x = n_mb;
  max_volume =x * 1024 *1024;
  return 0;

}

int daq_setmaxbuffersize (const int n_kb, std::ostream& os)
{
  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }

  Buffer1.setMaxSize(n_kb*1024);
  Buffer2.setMaxSize(n_kb*1024);
  return 0;
}

int daq_setadaptivebuffering (const int usecs, std::ostream& os)
{
  adaptivebuffering = usecs;
  return 0;
}

// this call is for later when we offer the
// option at run-time 
int daq_setEventFormat(const int f)
{
  int status =  Buffer1.setEventFormat(f);
  status |= Buffer2.setEventFormat(f);
  return status;
}

// this is a method for the devices to obtain
// the info which format we are writing
 
int daq_getEventFormat()
{
  return Buffer1.getEventFormat();
}

// elog server setup
int daq_set_eloghandler( const char *host, const int port, const char *logname)
{

  if ( ElogH) delete ElogH;
  ElogH = new ElogHandler (host, port, logname );

  setenv ( "DAQ_ELOGHOST", host , 1);
  char str [128];
  sprintf(str, "%d", port);
  setenv ( "DAQ_ELOGPORT", str , 1);
  setenv ( "DAQ_ELOGLOGBOOK", logname , 1);

  return 0;
}



int readn (int fd, char *ptr, const int nbytes)
{
  // in the interest of having just one "writen" call,
  // we determine if the file descriptor we got is a socket
  // (then we use send() ) or a file ( then we use write() ).  
  struct stat statbuf;
  fstat(fd, &statbuf);
  int fd_is_socket = 0;
  if ( S_ISSOCK(statbuf.st_mode) ) fd_is_socket = 1;

  int nread, nleft;
  nleft = nbytes;
  while ( nleft>0 )
    {
      if ( fd_is_socket) nread = recv (fd, ptr, nleft, MSG_NOSIGNAL);
      else nread = read (fd, ptr, nleft);

      if ( nread <= 0 ) 
	{
	  return nread;
	}

      nleft -= nread;
      ptr += nread;
    }

  return (nbytes-nleft);
}


int writen (int fd, char *ptr, const int nbytes)
{

  // in the interest of having just one "writen" call,
  // we determine if the file descriptor we got is a socket
  // (then we use send() ) or a file ( then we use write() ).  
  struct stat statbuf;
  fstat(fd, &statbuf);
  int fd_is_socket = 0;
  if ( S_ISSOCK(statbuf.st_mode) ) fd_is_socket = 1;

  int nleft, nwritten;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nwritten = 0;

      if ( fd_is_socket)  nwritten = send (fd, ptr, nleft, MSG_NOSIGNAL);
      else  nwritten = write (fd, ptr, nleft);
	  
      if ( nwritten <0 )
	{
	  perror ("writen");
	  return 0;
	}
      
      nleft -= nwritten;
      ptr += nwritten;
    }
  return (nbytes-nleft);
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
      pthread_mutex_lock(&M_cout);
      cout << " error opening file " << d << endl;
      perror ( d);
      pthread_mutex_unlock(&M_cout);

      *fd = 0;
      return -1;
    }

  *fd = ifd;
  CurrentFilename = d;
  PreviousFilename = CurrentFilename;

  file_is_open =1;

  return 0;
}

void *shutdown_thread (void *arg)
{

  unsigned long *t_args = (unsigned long *) arg;

  
  pthread_mutex_lock(&M_cout);
  cout << "shutting down... " <<  t_args[0] << "  " <<  t_args[1] << endl;
  if ( TriggerH) delete TriggerH;
  pthread_mutex_unlock(&M_cout);
  // unregister out service 
  svc_unregister ( t_args[0], t_args[1]);

  sleep(2);
  exit(0);
}


// this thread is watching for incoming requests for
// monitoring data

std::queue<int> fd_queue;  // this queue holds the monitoring requests 


void *monitorRequestwatcher_thread (void *arg)
{

  struct sockaddr_in server_addr;
  int sockfd;
  
  if ( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
      pthread_mutex_lock(&M_cout);
      cout  << "cannot create socket" << endl;
      pthread_mutex_unlock(&M_cout);
    }

  bzero( (char*)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(MONITORINGPORT);


  int status = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
  if (status < 0)
    {
      perror("bind");
    }
	
  pthread_mutex_lock(&M_cout);
  cout  << "Listening for monitoring requests on port " << MONITORINGPORT << endl;
  pthread_mutex_unlock(&M_cout);

  listen(sockfd, 16);

  int connection = 0;

  int dd_fd;
  struct sockaddr_in out;

  while (sockfd > 0)
    {

      socklen_t  len = sizeof(out); 
      dd_fd = accept(sockfd,  (struct sockaddr *) &out, &len);
      if ( dd_fd < 0 ) 
	{
	  pthread_mutex_lock(&M_cout);			
	  cout  << "error in accept socket" << endl;
	  pthread_mutex_unlock(&M_cout);
	}
      else
	{
	  char *host = new char[64]; 
	  
	  getnameinfo((struct sockaddr *) &out, sizeof(struct sockaddr_in), host, 64,
		      NULL, 0, NI_NOFQDN); 
	  
	  // time_t x =  time(0);
	  // pthread_mutex_lock(&M_cout);
	  // cout << ctime(&x) << "new request for monitoring connection accepted from " << host << endl;
	  // pthread_mutex_unlock(&M_cout);

	  pthread_mutex_lock(&FdManagementSem);
	  fd_queue.push(dd_fd);
	  pthread_mutex_unlock(&FdManagementSem);
	  
	  //kick off "the sender of monitoring data" thread
	  pthread_mutex_unlock(&MonitoringRequestSem);
	}
    }
  return 0;
}

void handler ( int sig)
{
  //pthread_mutex_lock(&M_cout);
  // cout  << " in handler..." << endl;
  //pthread_mutex_unlock(&M_cout);
  pthread_mutex_unlock(&SendProtectSem);
}  

void *sendMonitorData( void *arg)
{

  int fd;
  int status;
  
  while (1)
    {
      // we wait here until a monitoring request comes
      //pthread_mutex_lock(&MonitoringRequestSem);

      // when we get here, there are requestst. Now wait for a buffer
      // to be available:

      // pthread_mutex_lock(&M_cout);
      // cout <<  "  locking SendSem " << endl;
      // pthread_mutex_unlock(&M_cout);

      pthread_mutex_lock( &SendSem);

      // Now we go through all requests in the queue
      while ( !fd_queue.empty() )
	{
	  // now we are manipulating the queue. To do that, we need to
	  // lock-protect this:
	  
	  pthread_mutex_lock(&FdManagementSem);
	  fd = fd_queue.front();
	  fd_queue.pop();

	  pthread_mutex_unlock(&FdManagementSem);

	  int max_length =0;
	  	  
	  // we always wait for a controlword which tells us what to do next.
	  if ( (status = readn (fd, (char *) &max_length, 4) ) <= 0)
	    {
	      max_length = 0;
	    }
	  else
	    {
	      max_length = ntohl(max_length);

	      status = transportBuffer->sendData(fd, max_length);

	      int reply;
	      if ( ! status && ( status = readn (fd, (char *) &reply, 4) ) <= 0)
		{
		  // pthread_mutex_lock(&M_cout);
		  // time_t x =  time(0);
		  // cout << ctime(&x) << "  connection was broken for fd  " << fd << endl;
		  // pthread_mutex_unlock(&M_cout);
		}
	      else
		{
		  // reply = ntohl(reply);
		  // pthread_mutex_lock(&M_cout);
		  // cout  << " reply = " << reply << endl;
		  // pthread_mutex_unlock(&M_cout);
		}
	    }
	  close(fd);
	}
      // pthread_mutex_lock(&M_cout);
      // cout <<  "  unlocking SendProtectSem " << endl;
      // pthread_mutex_unlock(&M_cout);

      pthread_mutex_unlock(&SendProtectSem);
      if ( end_thread) pthread_exit(0);

    }
  return 0;
}

void *writebuffers ( void * arg)
{
  int i;


  while(1)
    {

      pthread_mutex_lock( &WriteSem); // we wait for an unlock


      last_bufferwritetime  = time(0);
      if ( outfile_fd) 
	{
	  unsigned int bytecount = transportBuffer->writeout(outfile_fd);
	  NumberWritten++;
	  BytesInThisRun += bytecount;
	}

      if ( end_thread) pthread_exit(0);
      
      pthread_mutex_unlock(&WriteProtectSem);
    }
  return 0;
}

int switch_buffer()
{

   // pthread_mutex_lock(&M_cout);
   // cout << __LINE__ << "  " << __FILE__ << " switching buffer" << endl;
   // pthread_mutex_unlock(&M_cout);

  daqBuffer *spare;
  
  pthread_mutex_lock(&WriteProtectSem);
  pthread_mutex_lock(&SendProtectSem);

  fillBuffer->addEoB();

  //switch buffers
  spare = transportBuffer;
  transportBuffer = fillBuffer;
  fillBuffer = spare;
  fillBuffer->prepare_next(++Buffer_number, TheRun);

  pthread_mutex_unlock(&WriteSem);
  pthread_mutex_unlock(&SendSem);
  return 0;

}

int daq_set_runnumberfile(const char *file)
{
  TheRunnumberfile = file;
  RunnumberfileIsSet = 1;
  FILE *fp = fopen(TheRunnumberfile.c_str(), "r");
  int r = 0;
  if (fp > 0)
    {
      int status = fscanf(fp, "%d", &r);
      if ( status != 1) r = 0; 
      if ( ! TheRun )
	{
	  TheRun = r;
	}
      fclose(fp);
    }

  return 0;
}

int daq_write_runnumberfile(const int run)
{
  if ( !RunnumberfileIsSet ) return 1;

  FILE *fp = fopen(TheRunnumberfile.c_str(), "w");
  if (fp > 0)
    {
      fprintf(fp, "%d\n", run);
      fclose(fp);
    }

  return 0;
}
  
int daq_set_filerule(const char *rule)
{
  TheFileRule = rule;
  return 0;
}

int daq_set_name(const char *name)
{
  MyName = name; 
  return 0;
}





// this is selecting from any of the existing run types 
int daq_setruntype(const char *type, std::ostream& os )
{
  std::string _type = type;
  std::map <string,string>::const_iterator iter = RunTypes.begin();
  for ( ; iter != RunTypes.end(); ++iter)
    {
      if ( iter->first == _type )
	{
	  TheFileRule = iter->second;
	  return 0;
	}
    }
  os << " Run type " << type << " is not defined " << endl;
  return 1;
}

int daq_getruntype(const int flag, std::ostream& os)
{
  std::map <string,string>::const_iterator iter = RunTypes.begin();
  for ( ; iter != RunTypes.end(); ++iter)
    {
      if ( iter->second == TheFileRule )
	{
	  if ( flag == 2) 
	    {
	      os << iter->first 	 << " - " << iter->second << endl;
	      return 0;
	    }
	  else
	    {
	      os << iter->first << endl;
	      return 0;
	    }
	}
    }
  return 0;
}

// this is defining a new run type (or re-defining an old one) 
int daq_define_runtype(const char *type, const char *rule)
{
  std::string _type = type;
  // std::map <string,string>::const_iterator iter = RunTypes.begin();
  // for ( ; iter != RunTypes.end(); ++iter)
  //   {
  //     if ( iter->first == _type )
  // 	{
  // 	  RunTypes[_type] = rule;
  // 	  return 0;
  // 	}
  //   }
  RunTypes[_type] = rule;
  return 0;
}

int daq_status_runtypes (std::ostream& os )
{
  os << " -- defined Run Types: ";
  return daq_list_runtypes(2, os);
}

int daq_list_runtypes(const int flag, std::ostream& os)
{

  if ( flag && RunTypes.size() == 0 )
    {
      os << " (none)" <<endl;
      return 0;
    }
  if (flag ==2) os << endl;

  std::map <string,string>::const_iterator iter = RunTypes.begin();
  for ( ; iter != RunTypes.end(); ++iter)
    {
      if (flag)
	{
	  os << "     " << setw(12) << iter->first 	 << " - " << iter->second << endl;
	}
      else
	{
	  os << iter->first << endl;
	}
      
    }
  return 0;
}    

int daq_get_name (std::ostream& os )
{
  os << MyName << endl;
  return 0;
}

std::string daq_get_myname()
{
  return MyName;
}

std::string& daq_get_filerule()
{
  return TheFileRule;
}

std::string& get_current_filename()
{
  return CurrentFilename;
}

std::string& get_previous_filename()
{
  return PreviousFilename;
}

int daq_begin(const int irun, std::ostream& os)
{
  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is already active" << endl;;
      return -1;
    }


  if (  irun ==0)
    {
      TheRun++;
    }
  else
    {
      TheRun = irun;
    }

  if ( daq_open_flag )
    {
      int status = open_file ( TheRun, &outfile_fd);
      if ( !status)
	{
	  if (ElogH) ElogH->BegrunLog( TheRun,"RCDAQ",  
				       get_current_filename());

	  daq_write_runnumberfile(TheRun);
	  last_bufferwritetime  = time(0);  // initialize this at begin-run

	}
      else
	{
	  os << "Could not open output file - Run " << TheRun << " not started" << endl;;
	  return -1;
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

  // a safety check: see that the buffers haven't been adjusted 
  // to a smaller value than the event size
  int wantedmaxsize = 0;
  for (int i = 0; i< MAXEVENTID; i++)
    {
      if ( (4*Eventsize[i] + 4*32) > fillBuffer->getMaxSize()
      	   || (4*Eventsize[i] + 4*32) > transportBuffer->getMaxSize() ) 
	{  
	  int x = 4*Eventsize[i] + 4*32;   // this is now in bytes
	  if ( x > wantedmaxsize ) wantedmaxsize = x;
	}
    }
  if ( wantedmaxsize)
    {
      if ( fillBuffer->setMaxSize(wantedmaxsize) || transportBuffer->setMaxSize(wantedmaxsize))
	{ 
	  os << "Cannot start run - event sizes larger than buffer, size " 
	     <<  wantedmaxsize/1024 << " Buffer size " 
	     << transportBuffer->getMaxSize()/1024 << endl;
	  return -1;
	}
      //      os << " Buffer size increased to " << transportBuffer->getMaxSize()/1024 << " KB"<< endl;
      
    }

  old_time =  StartTime = time(0);
  old_volume = 0;

      
  // here we sucessfully start a run. So now we set the env. variables
  char str[128];
  // RUNNUMBER
  sprintf( str, "%d", TheRun);
  setenv ( "DAQ_RUNNUMBER", str, 1);
  setenv ( "DAQ_FILERULE", TheFileRule.c_str() , 1);

  sprintf( str, "%ld", StartTime);
  setenv ( "DAQ_STARTTIME", str , 1);
	     
  if ( daq_open_flag )
    {
      setenv ( "DAQ_FILENAME", CurrentFilename.c_str() , 1);
    }

  fillBuffer->prepare_next(Buffer_number,TheRun);

  run_volume = 0;
  
  device_init();
  
  // readout the begin-run event
  readout(BEGRUNEVENT);
  
  //now enable the interrupts and reset the deadtime
  enable_trigger();


  os << "Run " << TheRun << " started" << endl;;
	  
  return 0;
}


int daq_end(std::ostream& os)
{
  if ( ! (Daq_Status & DAQ_RUNNING) ) 
    {
      os << "Run is not active" << endl;;
      return -1;
    }
  disable_trigger();
  device_endrun();

  Daq_Status ^= DAQ_RUNNING;
		
  readout(ENDRUNEVENT);
  switch_buffer();  // we force a buffer flush
		
  if ( file_is_open )
    {
      pthread_mutex_lock(&WriteProtectSem);
      pthread_mutex_unlock(&WriteProtectSem);

      double v = run_volume;
      v /= (1024*1024);

      if (ElogH) ElogH->EndrunLog( TheRun,"RCDAQ", Event_number, v, StartTime); 
      close (outfile_fd);
      outfile_fd = 0;
      file_is_open = 0;
 
    }
  os << "Run " << TheRun << " ended" << endl;

  unsetenv ("DAQ_RUNNUMBER");
  unsetenv ("DAQ_FILENAME");
  unsetenv ("DAQ_STARTTIME");

  Event_number = 0;
  run_volume = 0;    // volume in longwords 
  BytesInThisRun = 0;    // bytes actually written
  Buffer_number = 0;
  PreviousFilename = CurrentFilename;
  CurrentFilename = "";
  StartTime = 0;
  return 0;
}


void * daq_triggerloop (void * arg)
{
  int evttype;
  while (TriggerControl)
    {
      // let's se if we have a TriggerHelper object
      if (TriggerH)
	{
	  evttype = TriggerH->wait_for_trigger();
	  
	  if (evttype) 
	    {
	      Trigger_Todo=DAQ_READ;
	      CurrentEventType = evttype;
	      pthread_mutex_unlock ( &TriggerSem );

	      // pthread_mutex_lock(&M_cout);
	      // cout << __LINE__ << "  " << __FILE__ << " trigger, triggercontrol = " << TriggerControl  << endl;
	      //  pthread_mutex_unlock(&M_cout);
	      
	      pthread_mutex_lock ( &TriggerDone );

	      // pthread_mutex_lock(&M_cout);
	      // cout << __LINE__ << "  " << __FILE__ << " after lock triggercontrol = " << TriggerControl  << endl;
	      //  pthread_mutex_unlock(&M_cout);
	      
	    }

	}
      else
      	{
	  Trigger_Todo=DAQ_READ;
	  CurrentEventType = 1;
	  pthread_mutex_unlock ( &TriggerSem );

	  pthread_mutex_lock ( &TriggerDone );

      	  usleep (100000);
      	}
    }
   pthread_mutex_lock(&M_cout);
   cout << "trigger loop ended for run " << TheRun<< endl;
   pthread_mutex_unlock(&M_cout);
  return 0;
}



int daq_fake_trigger (const int n, const int waitinterval)
{
  int i;
  for ( i = 0; i < n; i++)
    {
      
      Trigger_Todo=DAQ_READ;
      pthread_mutex_unlock ( &TriggerSem );
      pthread_mutex_lock ( &TriggerDone );
	  
//       pthread_mutex_lock(&M_cout);
//       cout << "trigger" << endl;
//       pthread_mutex_unlock(&M_cout);

      usleep (200000);

    }
  return 0;
}


void * EventLoop( void *arg)
{
  int size;

  int go_on = 1;
  while (go_on)
    {

      pthread_mutex_lock ( &TriggerSem );

      if ( Daq_Status & DAQ_RUNNING ) 
	{
	  if  (Trigger_Todo == DAQ_READ)
	    {
	      Daq_Status |= DAQ_READING;
	      
	      int rstatus = readout(CurrentEventType);
	      Daq_Status ^= DAQ_READING;


	      rearm(DATAEVENT);
	      
	      // reset todo, and the DAQ_TRIGGER bit. 
	      Trigger_Todo = 0;
			  
	      reset_deadtime();
	      if (  rstatus)    // we got an endrun signal
		{
		  daq_end ( std::cout);
		  //go_on = 0;
		}
		  		  
		}
	}
      else
	// no, we are not running
	{
	  cout << "Run not active" << endl;
	  // reset todo, and the DAQ_TRIGGER bit. 
	  Trigger_Todo = 0;
	  reset_deadtime();
	}
    }
  
  return 0;
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
      //      cout << "calling init on "; 
      //(*d_it)->identify();
      (*d_it)->init();
    }

  return 0;
}

int device_endrun()
{

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      //      cout << "calling init on "; 
      //(*d_it)->identify();
      (*d_it)->endrun();
    }

  return 0;
}


int readout(const int etype)
{

  //  pthread_mutex_lock(&M_cout);
  // cout << " readout etype = " << etype << endl;
  //pthread_mutex_unlock(&M_cout);

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

  run_volume += 4*len;

  time_t now_time = time(0);
    
  if ( now_time - old_time > 10) // calculate avg speed
    {
      mb_per_second = ( run_volume - old_volume) / ( now_time - old_time) / 1024 /1204;
      //      cout << " mb/s: " << run_volume << " " << old_volume << "  " << now_time << "  " << old_time << "  " << mb_per_second << endl;
      old_volume = run_volume;
      old_time = now_time;
    }
  
  //  cout << "len, run_volume = " << len << "  " << run_volume << endl;

  int returncode = 0;

  if (  Daq_Status & DAQ_RUNNING )
    {
      if ( etype == DATAEVENT && max_volume > 0 && run_volume >= max_volume) 
	{
	  cout << " automatic end after " << max_volume /(1024*1024) << " Mb" << endl;
	  returncode = 1;
	}
      
      if ( etype == DATAEVENT && max_events > 0 && Event_number >= max_events ) 
	{
	  cout << " automatic end after " << max_events<< " events" << endl;
	  returncode = 1;
	}
    }

  if ( adaptivebuffering  &&  time(0) - last_bufferwritetime > adaptivebuffering )
    {
      switch_buffer();
      //      cout << "adaptive buffer switching" << endl;
    }

  return returncode;
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
      os << "Run is active" << endl;;
      return -1;
    }

  daq_open_flag =1;
  return 0;
}

int daq_shutdown(const unsigned long servernumber, const unsigned long versionnumber,
		 std::ostream& os)
{

  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;;
      return -1;
    }

  static unsigned long  t_args[2];
  t_args[0] = servernumber;
  t_args[1] = versionnumber;

  
  pthread_t t;

  int status = pthread_create(&t, NULL, 
			  shutdown_thread, 
			  (void *) t_args);
   
  if (status ) 
    {
      cout << "cannot shut down " << status << endl;
      os << "cannot shut down " << status << endl;
      return -1;
    }
  os << " ";
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
      os << "Run is active" << endl;;
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

int daq_clear_readlist(std::ostream& os)
{

  if ( Daq_Status & DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;;
      return -1;
    }

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      delete (*d_it);
    }

  DeviceList.clear();
  os << "Readlist cleared" << endl;

  return 0;
}


int rcdaq_init( pthread_mutex_t &M)
{

  int status;


  //  pthread_mutex_init(&M_cout, 0); 
  M_cout = M;

  pthread_mutex_init( &WriteSem, 0);
  pthread_mutex_init( &WriteProtectSem, 0);

  
  pthread_mutex_init( &MonitoringRequestSem, 0);
  pthread_mutex_init( &SendSem, 0);
  pthread_mutex_init( &SendProtectSem, 0);
  pthread_mutex_init( &FdManagementSem,0);

  pthread_mutex_init( &TriggerSem, 0);
  pthread_mutex_init( &TriggerDone, 0);

  // pre-lock them except the "protect" ones
  pthread_mutex_lock( &MonitoringRequestSem);
  pthread_mutex_lock( &WriteSem);
  pthread_mutex_lock( &SendSem);

  pthread_mutex_lock( &TriggerDone);
  pthread_mutex_lock( &TriggerSem);



  //  signal(SIGKILL, sig_handler);
  //signal(SIGTERM, sig_handler);
  //signal(SIGINT,  sig_handler);
   
  outfile_fd = 0;

  fillBuffer = &Buffer1;
  transportBuffer = &Buffer2;

#ifdef WRITEPRDF
  Buffer1.setEventFormat(DAQPRDFFORMAT);
  Buffer2.setEventFormat(DAQPRDFFORMAT);
#endif


  status = pthread_create(&ThreadId, NULL, 
			  writebuffers, 
			  (void *) 0);
   
  if (status ) 
    {
      cout << "error in write thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "write thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }

  status = pthread_create(&ThreadMon, NULL, 
			  sendMonitorData, 
			  (void *) 0);
     if (status ) 
    {
      cout << "error in send monitor data thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "monitor thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }
   
  status = pthread_create(&ThreadMon, NULL, 
			  monitorRequestwatcher_thread, 
			  (void *) 0);
     if (status ) 
    {
      cout << "error in send monitor data thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "monitor request thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }
   
  status = pthread_create(&ThreadEvt, NULL, 
			  EventLoop, 
			  (void *) 0);
   
  if (status ) 
    {
      cout << "error in event thread create " << status << endl;
      exit(0);
    }
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "event thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }
   
  //  std::ostringstream outputstream;
  //  daq_webcontrol ( ThePort, outputstream);
  daq_webcontrol ( ThePort);


  
  return 0;
}

int get_runnumber()
{
  if ( ! Daq_Status & DAQ_RUNNING ) return -1; 
  return TheRun;
}

int get_oldrunnumber()  // like get_runnumber, but doesn't return -1 when stopped
{
  return TheRun;
}

int get_eventnumber()
{
  if ( ! Daq_Status & DAQ_RUNNING ) return 0; 
  return Event_number;
}
double get_runvolume()
{
  if ( ! Daq_Status & DAQ_RUNNING ) return 0; 
  double v = run_volume;
  return v / (1024*1024);
}
int get_runduration()
{
  if ( ! Daq_Status & DAQ_RUNNING ) return 0; 
  return time(0) - StartTime;
}
int get_openflag()
{
  return daq_open_flag;
}


int daq_status (const int flag, std::ostream& os)
{

  double v = run_volume;
  v /= (1024*1024);

  switch (flag)
    {

    case STATUSFORMAT_SHORT:    // "short format"
      
      if ( Daq_Status & DAQ_RUNNING ) 
	{
	  os << TheRun  << " " <<  Event_number -1 << " " 
	     << v << " " 
	     << daq_open_flag << " " 
	     << get_current_filename() << " "
	     << time(0) - StartTime
	     << " \"" << MyName << "\"" << endl;
	}
      else
	{
	  os << "-1 0 0 " << daq_open_flag << " 0 0"
	     << " \"" << MyName << "\"" << endl;

	}
      
      break;

    case STATUSFORMAT_NORMAL:
      if ( Daq_Status & DAQ_RUNNING ) 
	{
	  os << "Run " << TheRun  
	     << " Event: " << Event_number 
	     << " Volume: " << v;
	  if ( daq_open_flag )
	    {
	      os << "  Logging enabled";
	    }
	  else
	    {
	      os << "  Logging disabled";
	    }	      
	  os<< endl;
	}
      else
	{
	  os << "Stopped";
	  if ( daq_open_flag )  os << " Logging enabled";
	  os<< endl;
	}
      break;

    default:  // flag 2++
      
      if ( Daq_Status & DAQ_RUNNING ) 
	{
	  os << "Running" << endl;
	  os << "Run Number:   " << TheRun  << endl;
	  os << "Event:        " << Event_number << endl;;
	  os << "Run Volume:   " << v << " MB"<< endl;
	  os << "Filename:     " << get_current_filename() << endl; 	  
	  os << "Duration:     " <<  time(0) - StartTime << " s" <<endl;
	  if (max_volume)
	    {
	      os << "Volume Limit: " <<  max_volume /(1024 *1024) << " Mb" << endl;
	    }
	  if (max_events)
	    {
	      os << "Event Limit:  " <<  max_events << endl;
	    }
	}
      else
	{
	  os << "Stopped"  << endl;
	  if ( daq_open_flag )
	    {
	      os << "Logging enabled"  << endl;
	    }
	  else
	    {
	      os << "Logging disabled"  << endl;
	    }
	  
	  if (max_volume)
	    {
	      os << "Volume Limit: " <<  max_volume /(1024 *1024) << " Mb" << endl;
	    }
	  if (max_events)
	    {
	      os << "Event Limit:  " <<  max_events << endl;
	    }
	  if ( TriggerH ) os << "have a trigger object" << endl;
	}
      os << "Filerule:     " <<  daq_get_filerule() << endl;
      os << "Buffer Sizes:     " <<  Buffer1.getMaxSize()/1024 << " KB";
      if ( adaptivebuffering) 
	{
	  os << " adaptive buffering: " << adaptivebuffering << " s"; 
	}
      os << endl;

      if ( ThePort)
      	{
      	  os << "Web control Port:  " << ThePort <<  endl;
      	}
      else
      	{
      	  os << "No Web control defined" << endl;
      	}


      if ( ElogH)
      	{
      	  os << "Elog:  " << ElogH->getHost() << " " << ElogH->getLogbookName() << " Port " << ElogH->getPort() <<  endl;
      	}
      else
      	{
      	  os << "Elog: not defined" << endl;
      	}

      daq_status_runtypes ( os);
      daq_status_plugin(flag, os);

      break;

    }

  return 0;
}

int daq_webcontrol(const int port, std::ostream& os)
{

  if (  port ==0)
    {
      ThePort=8899;
    }
  else
    {
      ThePort = port;
    }

  if ( ThreadWeb) // we had this thing running already 
    {
      mg_end();
      pthread_join(ThreadWeb, NULL);
      ThreadWeb = 0;
    }
  
  int status = pthread_create(&ThreadWeb, NULL, 
			  mg_server, 
			  (void *) &ThePort);
   
  if (status ) 
    {
      os << "error in web service creation " << status << endl;
      ThePort=0;
      return -1;
    }
  else
    {
      os << "web service created" << endl;
      return 0;
    }
  return 0;

}

int daq_getlastfilename( std::ostream& os)
{
  if (get_previous_filename().empty() ) return -1;
  os <<  get_previous_filename() << endl;
  return 0;

}


int daq_running()
{
  if ( Daq_Status & DAQ_RUNNING ) return 1;
  return 0;
}

