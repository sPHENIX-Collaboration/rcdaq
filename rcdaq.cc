//#define WRITEPRDF

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pthread.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

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
#include "md5.h"

#ifdef HAVE_MOSQUITTO_H
#include "MQTTConnection.h"
#endif

#define NR_THREADS 5


int open_file_on_server(const int run_number);
int server_open_Connection();
int open_serverSocket(const char * host_name, const int port);
int server_send_beginrun_sequence(const char * filename, const int runnumber, int fd);
int server_send_rollover_sequence(const char * filename, int fd);
int server_send_endrun_sequence(int fd);
int server_send_close_sequence(int fd);

void * mg_server (void *arg);
int mg_end();
int request_mg_update(const int what);


//pthread_mutex_t WriteSem;
//pthread_mutex_t WriteProtectSem;

pthread_mutex_t SizeUpdateSem;

pthread_mutex_t MonitoringRequestSem;
pthread_mutex_t SendSem;
pthread_mutex_t SendProtectSem;

pthread_mutex_t FdManagementSem;

char pidfilename[128];
char daq_event_env_string[128];

// those are the "todo" definitions. DAQ can be woken up by a trigger
// and read something, or by a command and change its status.

int servernumber = 0;


#define coutfl std::cout << __FILE__<< "  " << __LINE__ << " "
#define cerrfl std::cout << __FILE__<< "  " << __LINE__ << " "

int uservalues[8] = {0};

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
static std::string TheRunType = " ";
static std::string CurrentFilename = "";
static std::string PreviousFilename = "";

static std::string TheRunnumberfile;
static int RunnumberfileIsSet = 0;

static std::string TheRunnumberApp;
static int RunnumberAppIsSet = 0;

static std::string MyName = ""; 

static int daq_open_flag = 0;  //no files written unless asked for
static int daq_server_flag = 0;  //no server access
static int daq_server_port = 0;  // invalid port
static std::string daq_server_name = "";  // our server, if any

static unsigned int currentFillBuffernr = 0;
static unsigned int currentTransportBuffernr = 0;


// variables governing our write buffers
const auto processor_count = std::thread::hardware_concurrency();
static int nr_of_write_threads = 2;
static int current_data_format = 0;
static int compression_level = 0;
// the buffersize is in words, so 64MB
static int currentBufferSize = 16*1024*1024;  


static int file_is_open = 0;
static int server_is_open = 0;
static int current_filesequence = 0;
static int outfile_fd;
static int previous_outfile_fd;

pthread_mutex_t OutfileManagementSem;



static int md5_enabled =1;
static int md5_allow_turnoff = 0;

static int md5_enabled =1;
static int md5_allow_turnoff = 0;

#ifdef HAVE_MOSQUITTO_H
MQTTConnection *mqtt = 0;
std::string mqtt_host;
int mqtt_port = 0;
#endif

static md5_state_t md5state;


static int RunControlMode = 0;
static int CurrentEventType = 0;

static ElogHandler *ElogH =0;
static TriggerHandler  *TriggerH =0;


typedef std::vector<daq_device *> devicevector;
typedef devicevector::iterator deviceiterator;

#define MAXEVENTID 32
static int Eventsize[MAXEVENTID];

// int Daq_Status;

// #define DAQ_RUNNING  0x01
// #define DAQ_READING  0x02
// #define DAQ_ENDREQUESTED  0x04
// #define DAQ_PROTOCOL 0x10
// #define DAQ_BEGININPROGRESS  0x20

// the original bit-wise status word manipulation wasn't particular thread-safe.
// upgrading to individual variables (and we ditch "DAQ_READING")

int  DAQ_RUNNING = 0;
int  DAQ_ENDREQUESTED = 0;
int  DAQ_BEGININPROGRESS =0;

//static daqBuffer Buffer1;
//static daqBuffer Buffer2;

std::vector<daqBuffer *> daqBufferVector;
int last_written_buffernr = 0;
int last_bufferwritetime  = 0;

std::vector<unsigned int>  buffer_free;
std::vector<unsigned int>  buffer_inuse;
std::vector<unsigned int>  buffer_compressing;
std::vector<unsigned int>  buffer_wait_io;
std::vector<unsigned int>  buffer_writing;

int Trigger_Todo;
int Command_Todo;

int TheRun = 0;
time_t StartTime = 0;
int Buffer_number;

int Event_number;
int Event_number_at_last_open = 0;
int Event_number_at_last_end = 0;



int update_delta;

static int TriggerControl = 0;

int ThePort=8899;

int TheServerFD = 0;

daqBuffer  *fillBuffer, *transportBuffer;

pthread_t ThreadId;
pthread_t ThreadMon;
pthread_t ThreadEvt;
pthread_t ThreadWeb = 0;

int *thread_arg;

int end_thread = 0;

pthread_mutex_t M_cout;


struct sockaddr_in si_mine;

#define MONITORINGPORT 9930


devicevector DeviceList;



int NumberWritten = 0; 
unsigned long long BytesInThisRun = 0;
unsigned long long BytesInThisFile = 0;
unsigned long long RolloverLimit = 0;

unsigned long long run_volume, max_volume;
int max_events;

time_t last_speed_time = 0;
int last_event_nr = 0;

static time_t last_volume_time = 0;
double last_volume = 0;

int EvtId = 0;


int max_seconds = 0;
int verbose = 0;
int runnumber=1;
int packetid = 1001;
int max_buffers=0;

int adaptivebuffering = 15;

int persistentErrorCondition = 0;

std::string MyHostName;
std::string shortHostName;


std::map<int, int> fd_map;


int setup_buffer_histogram()
{


  buffer_free.clear();
  buffer_free.resize(daqBufferVector.size()+1,0);

  buffer_inuse.clear();
  buffer_inuse.resize(daqBufferVector.size()+1,0);

  buffer_compressing.clear();
  buffer_compressing.resize(daqBufferVector.size()+1,0);

  buffer_wait_io.clear();
  buffer_wait_io.resize(daqBufferVector.size()+1,0);

  buffer_writing.clear();
  buffer_writing.resize(daqBufferVector.size()+1,0);

  return 0;
}
  

int update_buffer_histogram()
{
  // to keep the def localized
  unsigned int nfree =0;
  unsigned int inuse =0;
  unsigned int compressing =0;
  unsigned int waitingforwrite =0;
  unsigned int writing =0;

  auto bitr = daqBufferVector.begin();
  int s =  (*bitr)->getStatus();
  if ( s== 0) nfree++;
  if (s & 0x1) inuse++;
  if (s & 0x4) compressing++;
  else if (s & 0x8) waitingforwrite++;
  else if (s & 0x10) writing++;
  
  bitr++;
  for (; bitr != daqBufferVector.end(); bitr++)
    {
      s =  (*bitr)->getStatus();
      if ( s== 0) nfree++;
      if (s & 0x1) inuse++;
      if (s & 0x4) compressing++;
      else if (s & 0x8) waitingforwrite++;
      else if (s & 0x10) writing++;
    }

  if ( nfree < buffer_free.size() ) buffer_free[nfree]++;
  //  else cerrfl << " wrong index " << nfree << " vecore size is " << buffer_free.size() <<  endl;

  if ( inuse < buffer_inuse.size() ) buffer_inuse[inuse]++;
  //else cerrfl << " wrong index"<< endl;

  if ( compressing < buffer_compressing.size() ) buffer_compressing[compressing]++;
  //else cerrfl << " wrong index"<< endl;

  if ( waitingforwrite < buffer_wait_io.size() ) buffer_wait_io[waitingforwrite]++;
  //else cerrfl << " wrong index"<< endl;

  if ( writing < buffer_writing.size() ) buffer_writing[writing]++;
  //else cerrfl << " wrong index"<< endl;
  return 0;

}

int print_buffer_histogram()
{
  cout << " ====== buffer statistics for run " << TheRun 
       << " compression level " << daqBufferVector[0]->getCompression() 
       << " nr_threads " << daqBufferVector.size() 
       << endl;


  cout << "  n    free   inuse    comp    wait writing" << endl;
  for ( int i = 0; i < buffer_free.size(); i++)
    {
      cout << setw(3) << i
	   << "  " << setw(6) <<  buffer_free[i]
	   << "  " << setw(6) <<  buffer_inuse[i]
	   << "  " << setw(6) <<  buffer_compressing[i]
	   << "  " << setw(6) <<  buffer_wait_io[i]
	   << "  " << setw(6) <<  buffer_writing[i]
	   << endl;
    }
  cout << " ====== end buffer statistics " << endl;
  return 0;
}

int UpdateLastWrittenBuffernr (const int n)
{
  last_written_buffernr = n;
  return 0;
}


int UpdateFileSizes (const unsigned long long size)
{
  pthread_mutex_lock( &SizeUpdateSem);

  BytesInThisRun += size;
  BytesInThisFile += size;

  pthread_mutex_unlock( &SizeUpdateSem);
  return 0;
}

int getCurrentOutputFD ()
{
  pthread_mutex_lock( &FdManagementSem);
  return outfile_fd;

}

void releaseOutputFD ()
{
  pthread_mutex_unlock( &FdManagementSem);

}


char *obtain_pidfilename()
{
 return pidfilename;
}

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

  TriggerControl = 0;
}


int reset_deadtime() 
{
  if ( TriggerH) TriggerH->rearm();
  return 0;
}

int enable_trigger() 
{

  TriggerControl=1;
  if ( TriggerH) TriggerH->enable();

  int status = pthread_create(&ThreadEvt, NULL, 
			  EventLoop, 
			  (void *) 0);
   
  if (status ) 
    {
      cout << "error in event thread create " << status << endl;
      exit(0);
    }
  else
    {
      if ( TriggerH) TriggerH->rearm();
    }

  return 0;
  
}

int disable_trigger() 
{
  TriggerControl=0;  // this makes the trigger process terminate
  if ( TriggerH) TriggerH->disable();
  
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

int daq_setRunControlMode(const int flag, std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }
  if (flag) RunControlMode = 1;
  else RunControlMode = 0;
  return 0;
}

int daq_setrolloverlimit (const int n_gb, std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }
  RolloverLimit = n_gb;
  return 0;

}

int daq_setmaxbuffersize (const int n_kb, std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }

  //Buffer1.setMaxSize(n_kb*1024);
  //Buffer2.setMaxSize(n_kb*1024);
  for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
    {
      (*it)->setMaxSize(n_kb*1024);
    }

  return 0;
}

int daq_define_buffersize (const int n_mb, std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << "Run is active" << endl;
      return -1;
    }

  int iret = 0;

  // the parameter means MB but we need units , so we need to divide by 4 to get what we need
  currentBufferSize = n_mb * 256* 1024;


  if ( currentBufferSize > daqBuffer::getAbsoluteMaxBuffersize() )
    {
      currentBufferSize = daqBuffer::getAbsoluteMaxBuffersize();
      os << "size " << n_mb << "MB too large, set to the max value " << currentBufferSize/(256* 1024) << "MB" <<  endl;
      //      coutfl  << "size " << n_mb << " too large, set to the max value " << currentBufferSize/(256* 1024) <<  endl;
      iret =1;
    }

  daq_set_number_of_write_threads ( daqBufferVector.size() );

  return iret;
}


int daq_setadaptivebuffering (const int usecs, std::ostream& os)
{
  adaptivebuffering = usecs;
  return 0;
}

// this call was added to allow chaging the format after the 
// server was started. Before this was a compile-time option.
int daq_setEventFormat(const int f, std::ostream& os )
{
  if ( daq_running() )
    {
      os << "Run is active" << endl;
      return -1;
    }
      
  if (DeviceList.size())
    {
      os << "Cannot switch format after devices are defined" << endl;
      return -1;
    }
  //  int status =  Buffer1.setEventFormat(f);
  //status |= Buffer2.setEventFormat(f);
  current_data_format = f;
  int status =0;
  for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
    {
      status |= (*it)->setEventFormat(current_data_format);
    }

  return status;
}

// this is a method for the devices to obtain
// the info which format we are writing
 
int daq_getEventFormat()
{
  //  return Buffer1.getEventFormat();
  return daqBufferVector[0]->getEventFormat();
}

int daq_getRunControlMode(std::ostream& os)
{
  os << RunControlMode << endl;
  return 0;
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
	  cerrfl << "fd nr = " << fd << endl;
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


  static char d[1024];
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

  cout << "opened file " << d << " fd nr " << ifd<< endl;

  
  *fd = ifd;
  CurrentFilename = d;
  PreviousFilename = CurrentFilename;
  md5_init(&md5state);

  file_is_open =1;
  
  // this is tricky. If we open the very first file, we latch in this event number.
  // but if we need to roll over, we find that out when the current buffer is written,
  // and the event nr is what's in this buffer, not the last written buffer.
  // only for the real start this is right:
  if (current_filesequence == 0)
    {
      Event_number_at_last_open = Event_number;
    }


  daq_generate_json (0); // generate the "new file" report
  
  return 0;
}

int open_file_on_server(const int run_number)
{
  
  if ( file_is_open) return -1;

  int status = server_open_Connection();
  if (status) return -1;

  static char d[1024];
  sprintf( d, TheFileRule.c_str(),
	   run_number, current_filesequence);

  status = server_send_beginrun_sequence(d, run_number, TheServerFD);
  if ( status)
    {
      return -1;
    }

  CurrentFilename = d;
  PreviousFilename = CurrentFilename;

  file_is_open =1;

  return 0;
}

// this function can be called sirectly from daq_end_interactive, 
// or we can make a thread for the "immediate" option 
void *daq_end_thread (void *arg)
{

  std::ostream *os = (std::ostream *) arg;

  // with an operator-induced daq_end, we make the event loop terminate
  // and wait for it to be done.

  disable_trigger();

  // it is possible that we call daq_end before we ever started a run
  if (ThreadEvt)   pthread_join(ThreadEvt, NULL);

  daq_end(*os);
  DAQ_ENDREQUESTED = 0;

  return 0;

}

// this function can be called sirectly from daq_end_interactive, 
// or we can make a thread for the "immediate" option 
void *daq_end_thread (void *arg)
{

  std::ostream *os = (std::ostream *) arg;

  // with an operator-induced daq_end, we make the event loop terminate
  // and wait for it to be done.

  disable_trigger();

  // it is possible that we call daq_end before we ever started a run
  if (ThreadEvt)   pthread_join(ThreadEvt, NULL);

  daq_end(*os);
  DAQ_ENDREQUESTED = 0;

  return 0;

}




void *shutdown_thread (void *arg)
{

  unsigned long *t_args = (unsigned long *) arg;

  
  pthread_mutex_lock(&M_cout);
  cout << "shutting down... " << endl; // <<  t_args[0] << "  " <<  t_args[1] << endl;
  int pid_fd = t_args[2];
  TriggerControl = 0;
  if ( TriggerH) delete TriggerH;
  pthread_mutex_unlock(&M_cout);
  // unregister out service 
  svc_unregister ( t_args[0], t_args[1]);

  flock(pid_fd, LOCK_UN);
  unlink(pidfilename);
  
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
  server_addr.sin_port = htons(MONITORINGPORT + servernumber);


  int status = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
  if (status < 0)
    {
      perror("bind");
    }
	
  pthread_mutex_lock(&M_cout);

  cout  << "Listening for monitoring requests on port " << MONITORINGPORT + servernumber << endl;

  pthread_mutex_unlock(&M_cout);

  listen(sockfd, 16);

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


int switch_buffer()
{

   // pthread_mutex_lock(&M_cout);
   // pthread_mutex_unlock(&M_cout);

  //daqBuffer *spare;
  
  //pthread_mutex_lock(&WriteProtectSem);
  pthread_mutex_lock(&SendProtectSem);

  fillBuffer->addEoB();
  fillBuffer->setDirty(1);

  //switch buffers
  // spare = transportBuffer;

  currentTransportBuffernr = currentFillBuffernr;
  transportBuffer = fillBuffer;

  

  // here we are implementing the ring with the daqBuffers 
  currentFillBuffernr++;
  if ( currentFillBuffernr >= daqBufferVector.size())
    {
      currentFillBuffernr = 0;
    }
  fillBuffer = daqBufferVector[currentFillBuffernr];

  //coutfl << "before wait_for free " << fillBuffer->getID() << " status is 0x" << hex << fillBuffer->getStatus() << dec << endl;
  
  update_buffer_histogram();

  
  fillBuffer->Wait_for_free();
  //coutfl << "After Wait_for_free on buffer " << fillBuffer->getID() << endl;
  
  fillBuffer->prepare_next(++Buffer_number, TheRun);

  // let's see if we need to roll over
    if ( daq_open_flag && RolloverLimit)
    {
      unsigned int blength = transportBuffer->getLength();
      //      coutfl << "Bytes in this fie " << BytesInThisFile << endl;

      if ( blength + BytesInThisFile > RolloverLimit * 1024 * 1024 * 1024) 
	{

	  if ( daq_server_flag)
	    {

	      current_filesequence++;
	      static char d[1024];
	      sprintf( d, TheFileRule.c_str(),
		       TheRun, current_filesequence);
	      
	      coutfl << " rolling over " << d << endl;

	      int status = server_send_rollover_sequence(d,TheServerFD);
	      CurrentFilename = d;
	      BytesInThisFile = 0;
	    }
	  else   // not server
	    {
	      
	      //coutfl << "  rolling over now " << endl;
	      // we wait until no one is using the open FD
	      pthread_mutex_lock( &FdManagementSem);
	      //coutfl << "  safe to close the file " << endl;
	      close(outfile_fd);
	      

	      //close_fd_except_active(outfile_fd);
	      file_is_open = 0;

	      current_filesequence++;
	      daq_generate_json(1);
	      
	      Event_number_at_last_open = daqBufferVector[last_written_buffernr]->getLastEventNumber() +1;
	      int status = open_file ( TheRun, &outfile_fd);
	      if (status)
		{
		  cout << MyHostName << "Could not open output file - Run " << TheRun << "  file sequence " << current_filesequence<< "  outfile_fd = " << outfile_fd << endl;
		}
	      BytesInThisFile = 0;
	      pthread_mutex_unlock( &FdManagementSem);
	    }
	  // cout << MyHostName << " -- Rolling output file over at "
	  //      << transportBuffer->getLength() + BytesInThisFile
	  //      << " sequence: " << current_filesequence
	  //      << " limit: " << RolloverLimit
	  //      << " now: " << CurrentFilename 
	  //      << endl;
	}
    }
  

  last_bufferwritetime  = time(0);
  if ( daq_open_flag &&  outfile_fd) 
    {
      //unsigned int bytecount = transportBuffer->getLength();
      
      transportBuffer->start_writeout_thread();
      NumberWritten++;
      //BytesInThisRun += bytecount;
      //BytesInThisFile += bytecount;
    }
  else if ( daq_server_flag &&  TheServerFD)
    {
      unsigned int bytecount = transportBuffer->sendout(TheServerFD);
      NumberWritten++;
      BytesInThisRun += bytecount;
      BytesInThisFile += bytecount;
    }
  else // not logging at all
    {
      //coutfl << " clearing buffer " << transportBuffer->getID() << endl;
      transportBuffer->setDirty(0);
    }

  //pthread_mutex_unlock(&WriteSem);
  pthread_mutex_unlock(&SendSem);
  return 0;

}

int daq_set_runnumberfile(const char *file, const int flag)
{
    if ( flag)
    {
      TheRunnumberfile.clear();
      RunnumberfileIsSet = 0;
      return 0;
    }

  TheRunnumberfile = file;
  RunnumberfileIsSet = 1;
  FILE *fp = fopen(TheRunnumberfile.c_str(), "r");
  int r = 0;
  if (fp)
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
  if ( RunControlMode ) return 1;

  FILE *fp = fopen(TheRunnumberfile.c_str(), "w");
  if (fp )
    {
      fprintf(fp, "%d\n", run);
      fclose(fp);
    }

  return 0;
}


int daq_set_runnumberApp(const char *file, const int flag)
{
  if ( flag)
    {
      TheRunnumberApp.clear();
      RunnumberAppIsSet = 0;
      return 0;
    }
  
  TheRunnumberApp = file;
  RunnumberAppIsSet = 1;
  return 0;
}

int getRunNumberFromApp()
{
  if ( ! RunnumberAppIsSet) return -1;
  FILE *fp = popen(TheRunnumberApp.c_str(),"r");
  if (fp == NULL)
    {
      std::cerr << "error running the runnumber app" << std::endl;
      return -1;
    }
    char in[64];
    int len = fread(in, 1, 64, fp);
    pclose(fp);
    
    std::stringstream s (in);
    int run;
    if (! (s >> run) )
      {
	return -1;
      }

  return run;
}


int daq_set_filerule(const char *rule , std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  TheFileRule = rule;
  return 0;
}

int daq_set_name(const char *name)
{
  MyName = name;
  if (servernumber)
    {
      MyName = MyName + ":" + to_string(servernumber);
    }
  request_mg_update (MG_REQUEST_NAME);
  return 0;
}


#ifdef HAVE_MOSQUITTO_H
int daq_set_mqtt_host(const char * host, const int port, std::ostream& os)
{
  coutfl <<  " mqtt host " << host << " port " << port << endl;
  if (mqtt) delete mqtt;

  if (strcasecmp(host, "None") == 0) // delete existing def
    {
      mqtt = 0;
      mqtt_host = "";
      mqtt_port = 0;
      return 0;
    }

  mqtt_host = host;
  mqtt = new MQTTConnection(mqtt_host, "rcdaq", port);
  if ( mqtt->Status())
    {
      delete mqtt;
      mqtt =0;
      mqtt_host = "";
      mqtt_port = 0;
      os << "Could not connect to host " << host << " on port " << port << endl;
      return 1;
    }

  mqtt_host = host;
  mqtt_port = port;

  return 0;
}

int daq_get_mqtt_host(std::ostream& os)
{
  if (!mqtt)
    {
      os << " No MQTT host defined" << endl;
      return 1;
    }
  os << " Host " << mqtt->GetHostName() << " " << " port " << mqtt->GetPort() << endl; 
  return 0;
}
#endif



// this is selecting from any of the existing run types 
int daq_setruntype(const char *type, std::ostream& os )
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  std::string _type = type;
  std::map <string,string>::const_iterator iter = RunTypes.begin();
  for ( ; iter != RunTypes.end(); ++iter)
    {
      if ( iter->first == _type )
	{
	  TheFileRule = iter->second;
	  TheRunType = _type;
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
  os << "  -- defined Run Types: ";
  return daq_list_runtypes(2, os);
}

int daq_list_runtypes(const int flag, std::ostream& os)
{

  if ( flag && RunTypes.size() == 0 )
    {
      os << "  (none)" <<endl;
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

double daq_get_mb_per_second()
{

  time_t now_time = time(0);
  if ( now_time == last_volume_time) return 0;
  double time_delta = now_time - last_volume_time;
  double mb_per_second = ( run_volume - last_volume) / time_delta / 1024. /1204.;
  last_volume = run_volume;
  last_volume_time = now_time;
  return mb_per_second;
}

double daq_get_events_per_second()
{
  time_t now_time = time(0);
  if ( now_time == last_speed_time) return 0;
  double time_delta = now_time - last_speed_time;
  double events_per_second = ( Event_number - last_event_nr) / time_delta;
  last_event_nr = Event_number;
  last_speed_time = now_time;
  return events_per_second;
}


void * daq_begin_thread( void *arg)
{
  int irun = *(int*)arg;
  int status = daq_begin( irun, std::cout);
  if (status)
    {
      // not sure what to do exactly
      coutfl << " asynchronous begin run failed" << endl;
    }

  DAQ_BEGININPROGRESS = 0;

  return 0;
}

int daq_begin_immediate(const int irun, std::ostream& os)
{
  static unsigned int  b_arg;
  b_arg = irun;

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;
      return -1;
    }
  if ( irun )
    {
      os << MyHostName << "Run " << irun  << " begin requested" << endl;
    }
  else   // I need to think about this a bit. We let the begin_run update the run number. 
    {
      os << MyHostName << "Run " << TheRun+1 << " begin requested" << endl;
    }
  
  DAQ_BEGININPROGRESS = 1;
  
  pthread_t t;
 
  int status = pthread_create(&t, NULL,
                          daq_begin_thread,
                          (void *) &b_arg);
  if (status ) 
    {

      cout << "begin_run failed " << status << endl;
      os << "begin_run failed " << status << endl;
      return -1;
    }
  return 0;
}


int daq_begin(const int irun, std::ostream& os)
{
  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is already active" << endl;;
      return -1;
    }

  if ( RunControlMode &&  irun ==0 )
    {
      os << MyHostName << " No automatic Run Numbers in Run Control Mode" << endl;;
      return -1;
    }
  
  if ( persistentErrorCondition)
    {
      os << MyHostName << "*** Previous error with server connection" << endl;;
      return -1;
    }
    
  if (ThreadEvt) pthread_join(ThreadEvt, NULL);

  setup_buffer_histogram();
  
  // set the status to "running"
  DAQ_RUNNING = 1;
  current_filesequence = 0;
  
  // if we are in run Control mode, we don't allow automatic run numbers

  if (  irun ==0)
    {
      if ( RunnumberAppIsSet)
	{
	  int run = getRunNumberFromApp();
	  if ( run < 0)
	    {
	      os << MyHostName << "Could not obtain a run number from " << TheRunnumberApp << ", run  not started" << endl;
	      DAQ_RUNNING = 0;
	      return -1;
	    }
	  TheRun = run;
	}
      else
	{
	  TheRun++;
	}
    }
  else
    {
      TheRun = irun;
    }

  //initialize the Buffer and event number
  Buffer_number = 1;
  Event_number  = 1;
  Event_number_at_last_open = 0;
  Event_number_at_last_end = 0;

  currentFillBuffernr = 0;
  currentTransportBuffernr = 0;
  fillBuffer = daqBufferVector[currentFillBuffernr];
  transportBuffer = daqBufferVector[currentTransportBuffernr];

  
  // initialize the run/file volume
  BytesInThisRun = 0;    // bytes actually written
  BytesInThisFile = 0;

  
  if ( daq_open_flag)
    {
      if ( daq_server_flag)
	{
      
	  int status = open_file_on_server(TheRun);
	  if ( !status)
	    {

	      if (ElogH) ElogH->BegrunLog( TheRun,"RCDAQ",  
					   get_current_filename());

	      daq_write_runnumberfile(TheRun);
	      last_bufferwritetime  = time(0);  // initialize this at begin-run
	      
	    }
	  else
	    {
	      os << MyHostName << "Could not open remote output file - Run " << TheRun << " not started" << endl;;
	      DAQ_RUNNING = 0;
	      return -1;
	    }
	}

      else  // we log to a standard local file
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
	      os << MyHostName << "Could not open output file - Run " << TheRun << " not started" << endl;;
	      DAQ_RUNNING = 0;
	      return -1;
	    }
	}

      
    }



  // just to be safe, clear the "end requested" bit
  DAQ_ENDREQUESTED = 0;
  
  cout << "starting run " << TheRun << " at " << time(0) << endl; 
  set_eventsizes();


  
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
	  os << MyHostName << "Cannot start run - event sizes larger than buffer, size " 
	     <<  wantedmaxsize/1024 << " Buffer size " 
	     << transportBuffer->getMaxSize()/1024 << endl;
	  DAQ_RUNNING = 0;
	  return -1;
	}
      //      os << " Buffer size increased to " << transportBuffer->getMaxSize()/1024 << " KB"<< endl;
      
    }

  last_volume_time = last_speed_time =  StartTime = time(0);
  last_event_nr = 0;
  last_volume = 0;

      
  // here we sucessfully start a run. So now we set the env. variables
  char str[128];
  // RUNNUMBER
  sprintf(str, "%d", TheRun);
  setenv ( "DAQ_RUNNUMBER", str , 1);

  // sprintf( str, "%d", Event_number);
  // setenv ( "DAQ_EVENTNUMBER", str , 1);
  
  setenv ( "DAQ_FILERULE", TheFileRule.c_str() , 1);

  sprintf( str, "%ld", StartTime);
  setenv ( "DAQ_STARTTIME", str , 1);
	     
  if ( daq_open_flag || daq_server_flag )
    {
      setenv ( "DAQ_FILENAME", CurrentFilename.c_str() , 1);
    }

  daqBuffer::reset_count();

  // we are opening a new file here, so we restart the MD5 calculation 
  md5_init(&md5state);
  
  fillBuffer->prepare_next(Buffer_number,TheRun);

  run_volume = 0;
  
  device_init();
  
  // readout the begin-run event
  readout(BEGRUNEVENT);
  
  //now enable the interrupts and reset the deadtime
  enable_trigger();

  request_mg_update (MG_REQUEST_SPEED);

  os << MyHostName << "Run " << TheRun << " started" << endl;

  return 0;
}

int daq_end_immediate(std::ostream& os)
{
  if ( ! (DAQ_RUNNING) ) 
    {
      os << MyHostName << "Run is not active" << endl;;
      return -1;
    }
  os << MyHostName << "Run " << TheRun << " end requested" << endl;
  DAQ_ENDREQUESTED = 1;

  pthread_t t;
  
  void * x = &os;


  //coutfl << " calling end_thread as thread" << endl; 

  int status = pthread_create(&t, NULL,
                          daq_end_thread,
                          x);
  if (status ) 
    {

      cout << "end_run failed " << status << endl;
      os << "end_run failed " << status << endl;
    }

  //coutfl << " done setting up  end_thread" << endl; 

  return 0;
}


// this function is to hold further interactions until a asynchronous begin-run is over 
int daq_wait_for_begin_done()
{
    while ( DAQ_BEGININPROGRESS ) usleep(10000);
    return 0;
}

// this function is to avoid a race condition with the asynchronous "end requested" feature
int daq_wait_for_actual_end()
{

  while ( DAQ_ENDREQUESTED ) 
    {
      usleep(10000);
    }

  return 0;
}

int daq_end_interactive(std::ostream& os)
{

  void *x = &os;
  daq_end_thread (x);
  return 0;
}

int daq_end(std::ostream& os)
{
  
  if ( ! (DAQ_RUNNING) ) 
    {
      os << MyHostName << "Run is not active" << endl;;
      return -1;
    }
  
  disable_trigger();
  device_endrun();

  readout(ENDRUNEVENT);
  
  // for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
  //   {
  //     (*it)->setVerbosity(1);
  //   }
  
  
  switch_buffer();  // we force a buffer flush

    print_buffer_histogram();

  for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
    {
      //coutfl << "Waiting for " << (*it)->getID() << " status is " << (*it)-> getDirty() << endl;
      while ( (*it)->getDirty() )
	{
	  usleep(200);
	}
      //coutfl << "Done waiting for " << (*it)->getID() << endl;
    }

  if ( file_is_open  )
    {

      //      pthread_mutex_lock(&WriteProtectSem);
      // pthread_mutex_unlock(&WriteProtectSem);

      //coutfl << "moving on..."  << endl;


      double v = run_volume;
      v /= (1024*1024);
      
      if (ElogH) ElogH->EndrunLog( TheRun,"RCDAQ", Event_number, v, StartTime);
      
      if ( daq_server_flag)
	{
	  server_send_endrun_sequence(TheServerFD);
	  if ( server_send_close_sequence(TheServerFD) )
	    {
	      coutfl << " error in closing connection...   " << std::endl;
	    }
	  close(TheServerFD);
	  TheServerFD = 0;
	}
      else
	{
	  while (1)
	    {
	      int dirtycount = 0;
	      for (auto it: daqBufferVector)
		{
		  if ( it->getDirty()) dirtycount++;
		}
	      if ( dirtycount == 0) break;
	    }


	  cout << "closing outfile.. " << endl;
	  //outfile_fd = 0; // mark all files inactive
	  //close_fd_except_active(outfile_fd);

	  
	  int still_dirty = 1;
	  while ( still_dirty)
	    {
	      still_dirty = 0;
	      for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
		{
		  if ( (*it)->getDirty() ) still_dirty=1; 
		}
	    }
	  close(outfile_fd);
	  outfile_fd = 0;

	  daq_generate_json(1);
	}
      file_is_open = 0;


    } 


  
  cout << MyHostName <<  "Run " << TheRun << " ended at " << time(0) << " with " << get_eventnumber() -1 << " events " << endl; 

  unsetenv ("DAQ_RUNNUMBER");
  unsetenv ("DAQ_FILENAME");
  unsetenv ("DAQ_STARTTIME");

  Event_number_at_last_end = Event_number;
  Event_number = 0;
  run_volume = 0;    // volume in longwords 
  BytesInThisRun = 0;    // bytes actually written
  BytesInThisFile = 0;
  Buffer_number = 0;
  PreviousFilename = CurrentFilename;
  CurrentFilename = "";
  StartTime = 0;
  DAQ_RUNNING = 0;

  last_volume_time = last_speed_time = time(0);
  last_event_nr = 0;
  last_volume = 0;

  request_mg_update (MG_REQUEST_SPEED);

  DAQ_ENDREQUESTED = 0;
  os << MyHostName <<  "Run " << TheRun << " ended" << endl;

  return 0;
}

int daq_fake_trigger (const int n, const int waitinterval)
{
  int i;
  for ( i = 0; i < n; i++)
    {
      
      Trigger_Todo=DAQ_READ;
	  
//       pthread_mutex_lock(&M_cout);
//       cout << "trigger" << endl;
//       pthread_mutex_unlock(&M_cout);

      usleep (200000);

    }
  return 0;
}


void * EventLoop( void *arg)
{

  // pthread_mutex_lock(&M_cout);
  //  std::cout << __FILE__ << " " << __LINE__ << " event loop starting...   " << std::endl;
  // pthread_mutex_unlock(&M_cout);

  int rstatus;
  
  while (TriggerControl)
    {

      // let's see if we have a TriggerHelper object
      if (TriggerH)
	{
	  CurrentEventType = TriggerH->wait_for_trigger();
	}
      else // we auto-generate a few triggers
	{
	  CurrentEventType = 1;
	  usleep (100000);
	}
      
      
      if (CurrentEventType) 
	{
	  
	  if ( DAQ_RUNNING ) 
	    {
	      
	      rstatus = readout(CurrentEventType);
	      
	      if (  rstatus)    // we got an endrun signal
		{
		  TriggerControl = 0;
		  //reset_deadtime();

		  //cout << __LINE__ << " calling daq_end" << endl;
		  daq_end ( std::cout);
		}
	      else
		{
		  rearm(DATAEVENT);
		  reset_deadtime();
		}
	    }
	  else  // no, we are not running
	    {
	      cout << "Run not active" << endl;
	      // reset todo, and the DAQ_TRIGGER bit. 
	      TriggerControl = 0;
	    }
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
  // pthread_mutex_unlock(&M_cout);

  int len = EVTHEADERLENGTH;

  if (etype < 0 || etype>MAXEVENTID) return 0;

  deviceiterator d_it;

  sprintf( daq_event_env_string, "DAQ_EVENTNUMBER=%d", Event_number);
  
  int status = fillBuffer->nextEvent(etype,Event_number, Eventsize[etype]);
  if (status != 0) 
    {
      switch_buffer();
      status = fillBuffer->nextEvent(etype,Event_number,  Eventsize[etype]);
    }

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      len += fillBuffer->addSubevent ( (*d_it) );
    }

  Event_number++;
  Event_number_at_last_end = Event_number;


  run_volume += 4*len;

  int returncode = 0;

  if (  DAQ_RUNNING )
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

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  // if ( daq_server_flag)
  //   {
  //     os << "Server logging is enabled" << endl;;
  //     return -1;
  //   }
      
  persistentErrorCondition = 0;

  daq_open_flag =1;
  return 0;
}

int daq_set_compression (const int level, std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  compression_level = level;
  for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
    {
      (*it)->setCompression(compression_level);
    }

  return 0;
}

int daq_set_nr_threads (const int n, std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  int nr = daq_set_number_of_write_threads(n);
  if ( nr != n)
    {
      os << "number of threads cannot be less than 2 or more than the number of cores-1 (" << processor_count -1 << ") -  set to "<<  nr << endl;
    }
  return 0;
}

int daq_set_md5enable (const int flag, std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }
  if ( ! md5_allow_turnoff) 
    {
      os << MyHostName << " MD5 switchoff not enabled" << endl;;
      return 0;
    }

  if (flag)
    {
      for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
	{
	  (*it)->setMD5Enabled(1);
	}
      md5_enabled = 1;
      os << MyHostName << " MD5 enabled " << endl;;
    }
  else
    {
      for ( auto it = daqBufferVector.begin(); it!= daqBufferVector.end(); ++it)
	{
	  (*it)->setMD5Enabled(0);
	}
      md5_enabled = 0;
      os << MyHostName << " MD5 disabled " << endl;;
    }

  return 0;
}


int daq_set_md5enable (const int flag, std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }
  if ( ! md5_allow_turnoff) 
    {
      os << MyHostName << " MD5 switchoff not enabled" << endl;;
      return 0;
    }

  if (flag)
    {
      Buffer1.setMD5Enabled(1);
      Buffer2.setMD5Enabled(1);
      md5_enabled = 1;
    }
  else
    {
      Buffer1.setMD5Enabled(0);
      Buffer2.setMD5Enabled(0);
      md5_enabled = 0;
    }

  return 0;
}

int daq_set_md5allowturnoff (const int flag, std::ostream& os)
{
  if (flag)
    {
      md5_allow_turnoff = 1;
      os << MyHostName << "MD5 turnoff allowed" << endl;
    }
  else 
    {
      md5_allow_turnoff = 0;
      os << MyHostName << "MD5 turnoff not allowed" << endl;
    }
  return 0;
}

int daq_set_server (const char *hostname, const int port, std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  daq_server_name = hostname;
  if ( daq_server_name == "None")
    {
      daq_server_flag = 0;
      daq_server_name = "";      
      daq_server_port = 0;
      return 0;
    }
  
  daq_server_flag = 1;
  daq_server_port = port;
  if ( ! daq_server_port) daq_server_port = 5001;

  return 0;
}

  

int server_open_Connection()
{
    
  if ( !daq_server_flag)
    {
      return -1;
    }

  persistentErrorCondition = 0;

  int theport = daq_server_port;
  if ( ! theport) theport = 5001;
  
  TheServerFD = open_serverSocket(daq_server_name.c_str(), theport);
  if ( TheServerFD < 0)
    {
      if ( TheServerFD == -1) 
	{
	  cout << __FILE__<< " " << __LINE__  << " error connecting to server " << daq_server_name << " on port " << theport << endl;
	}
      else
	{
	   cout << __FILE__<< " " << __LINE__ << " error connecting to server " << daq_server_name << " on port " << theport << "  " << gai_strerror(TheServerFD) << endl;
	}

      TheServerFD = 0;
      persistentErrorCondition = 1;
      return -1;
    }
  
  return 0;
}


int daq_shutdown(const unsigned long servernumber, const unsigned long versionnumber, const int pid_fd,
		 std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  if (daq_server_flag)
    {
      daq_close(std::cout);
    }
  
  static unsigned long  t_args[3];
  t_args[0] = servernumber;
  t_args[1] = versionnumber;
  t_args[3] = pid_fd;
    
  pthread_t t;

  int status = pthread_create(&t, NULL, 
			  shutdown_thread, 
			  (void *) t_args);
   
  if (status ) 
    {
      cout << "cannot shut down " << status << endl;
      os << MyHostName << "cannot shut down " << status << endl;
      return -1;
    }
  os << " ";
  return 0;
}

int is_open()
{
  return daq_open_flag;
}

int is_server_open()
{
  return daq_server_flag;
}

int daq_close (std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  daq_open_flag =0;
  persistentErrorCondition = 0;

  return 0;
}

int daq_server_close (std::ostream& os)
{

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }
  if ( server_send_close_sequence(TheServerFD) )
    {
      coutfl << " error in closing connection...   " << std::endl;
    }
  close(TheServerFD);
  TheServerFD = 0;
  
  daq_server_flag =0;
  return 0;
}


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

  if ( DAQ_RUNNING ) 
    {
      os << MyHostName << "Run is active" << endl;;
      return -1;
    }

  deviceiterator d_it;

  for ( d_it = DeviceList.begin(); d_it != DeviceList.end(); ++d_it)
    {
      delete (*d_it);
    }

  DeviceList.clear();
  os << MyHostName << "Readlist cleared" << endl;

  return 0;
}


int rcdaq_init( const int snumber, pthread_mutex_t &M)
{

  coutfl << " Number of cores: " << processor_count << endl;
  
  int status;

  servernumber = snumber;
  ThePort += servernumber;
    
  char hostname[HOST_NAME_MAX];
  status = gethostname(hostname, HOST_NAME_MAX);
  if (!status)
    {
      shortHostName = hostname;
      MyHostName = hostname;
      if (servernumber)
	{
	  shortHostName = shortHostName + ":" + to_string(servernumber);
	  MyHostName = MyHostName + ":" + to_string(servernumber);
	}
      MyHostName += " - ";
    }
  MyName = shortHostName;

  sprintf( daq_event_env_string, "DAQ_EVENTNUMBER=%d", -1);
  putenv(daq_event_env_string);



  //  pthread_mutex_init(&M_cout, 0); 
  M_cout = M;

  pthread_mutex_init( &SizeUpdateSem, 0);

  pthread_mutex_init( &MonitoringRequestSem, 0);
  pthread_mutex_init( &SendSem, 0);
  pthread_mutex_init( &SendProtectSem, 0);
  pthread_mutex_init( &FdManagementSem,0);
  pthread_mutex_init( &OutfileManagementSem, 0);

  // pre-lock them except the "protect" ones
  pthread_mutex_lock( &MonitoringRequestSem);
  pthread_mutex_lock( &SendSem);

  ThreadEvt = 0;

  outfile_fd = 0;

  daq_set_number_of_write_threads(2); // we start with 2 as we had before
  

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
   
   
  //  std::ostringstream outputstream;
  //  daq_webcontrol ( ThePort, outputstream);
  daq_webcontrol ( ThePort);


  
  return 0;
}

int daq_set_number_of_write_threads(const int n)
{
  // first, clear the buffer vector

  for ( auto it :  daqBufferVector)
    {
      delete it;
    }
  daqBufferVector.clear();

  nr_of_write_threads = n;

  // sanity checks
  if ( n < 2)
    {
      coutfl << " number of threads cannot be less than 2" << endl;
      nr_of_write_threads = 2;
    }
  
    if ( n > processor_count -1)
      {
	coutfl << " number of threads cannot be more than the number of cores -1 (" << processor_count -1 << ")" << endl;
	nr_of_write_threads = processor_count -1;
      }
  
  for (int i = 0; i < nr_of_write_threads ; i++)
    {
      daqBufferVector.push_back(new daqBuffer (1, currentBufferSize) );
    }


  for ( int i =0; i < daqBufferVector.size(); i++)
    {
      daqBufferVector[i]->setID(i);
      daqBufferVector[i]->setMD5State(&md5state);
      daqBufferVector[i]->setEventFormat(current_data_format);
      daqBufferVector[i]->setMD5Enabled(md5_enabled);
      daqBufferVector[i]->setCompression(compression_level);
    }

  // generate the circular buffer, note the start at 1
  for ( int i =1; i < daqBufferVector.size(); i++)
    {
      daqBufferVector[i]->setPreviousBuffer(daqBufferVector[i-1]);
    }
  daqBufferVector[0]->setPreviousBuffer(daqBufferVector[daqBufferVector.size()-1]);

  return nr_of_write_threads;
}


int get_runnumber()
{
  if ( ! DAQ_RUNNING ) return -1; 
  return TheRun;
}

int get_oldrunnumber()  // like get_runnumber, but doesn't return -1 when stopped
{
  return TheRun;
}

int get_eventnumber()
{
  if ( !  DAQ_RUNNING ) return 0; 
  return Event_number;
}
double get_runvolume()
{
  if ( ! DAQ_RUNNING ) return 0; 
  double v = run_volume;
  return v / (1024*1024);
}
int get_runduration()
{
  if ( ! DAQ_RUNNING ) return 0; 
  return time(0) - StartTime;
}

int get_openflag()
{
  return daq_open_flag;
}
int get_serverflag()
{
  return daq_server_flag;
}


int daq_status( const int flag, std::ostream& os)
{

  double volume = run_volume;
  volume /= (1024*1024);

  switch (flag)
    {

    case 100: // special case - make json output 
      
      if ( DAQ_RUNNING ) 
	{
	  os << "{\"state\":\"running\",";
	}
      else
	{
	  os << "{\"state\":\"stopped\",";
	}


      os << " \"runnumber\":" << TheRun << ","
	 << " \"host\":\"" << shortHostName << "\","
	 << " \"runtype\":\"" << TheRunType << "\","
	 << " \"event\":\"" << Event_number -1 << "\","
	 << " \"volume\":\"" << volume << "\","
	 << " \"Filerule\":\"" << daq_get_filerule() << "\","
	 << " \"Filename\":\"" << get_current_filename() << "\","
	 << " \"Rolloverlimit\":\"" << RolloverLimit << "\","
	 << " \"nr_write_threads\":\"" << nr_of_write_threads << "\"," 
	 << " \"BufferMaxsize\":\"" << daqBufferVector[0]->getMemSize()/256/1024 << "\"," 
	 << " \"Buffersize\":\"" << daqBufferVector[0]->getMaxSize() << "\"," ;

      if (daqBufferVector[0]->getCompression() )
	{
	  os << "\"compression\":\"enabled\" ,";
	}
      else
	{
	  os << "\"compresssion\":\"disabled\" ,";
	}

      os << " \"level\":\"" << daqBufferVector[0]->getCompression() << "\"," ;


      if ( daqBufferVector[0]->getMD5Enabled() )
	{
	  os << "\"MD5\":\"enabled\" ,";
	}
      else
	{
	  os << "\"MD5\":\"disabled\" ,";
	}

      

      if ( DAQ_RUNNING ) 
	{
	  os << " \"Duration\":\"" << time(0) - StartTime << "\",";
	}
      else
	{
	  os << " \"Duration\":\"" << 0 << "\",";
	}
      os << " \"VolumeLimit\":\"" << max_volume /(1024 *1024) << "\","
	 << " \"EventLimit\":\"" << max_events << "\",";


      { // to keep the def localized
	int total =0;
	int nfree =0;
	int inuse =0;
	int compressing =0;
	int waitingforwrite =0;
	int writing =0;

	auto bitr = daqBufferVector.begin();
	int s =  (*bitr)->getStatus();
	total++;
	if ( s== 0) nfree++;
	if (s & 0x1) inuse++;
	if (s & 0x4) compressing++;
	else if (s & 0x8) waitingforwrite++;
	else if (s & 0x10) writing++;

	bitr++;
	for (; bitr != daqBufferVector.end(); bitr++)
	  {
	    total++;
	    s =  (*bitr)->getStatus();
	    if ( s== 0) nfree++;
	    if (s & 0x1) inuse++;
	    if (s & 0x4) compressing++;
	    else if (s & 0x8) waitingforwrite++;
	    else if (s & 0x10) writing++;

	  }
	os << " \"Bufferstatus\":[" 
	   << "{\"total\":" << total << "}"  
	   << ",{\"free\":" << nfree << "}"  
	   << ",{\"inuse\":" << inuse << "}"  
	   << ",{\"compressing\":" << compressing << "}"  
	   << ",{\"wait_io\":" << waitingforwrite << "}"  
	   << ",{\"writing\":" << writing << "}"  
	   << "] }" << endl;
      }

      break;

    case STATUSFORMAT_SHORT:    // "short format"
      
      if ( DAQ_RUNNING ) 
	{
	  os << TheRun  << " " <<  Event_number -1 << " " 
	     << volume << " ";
	  os  << daq_open_flag << " ";
	  os  << daq_server_flag << " ";
	  os  << time(0) - StartTime << " ";
	  os << get_current_filename() << " "
	     << " \"" << MyName << "\"" << endl;
	}
      else
	{
	  os << "-1 0 0 ";
	  os  << daq_open_flag << " ";
	  os  << daq_server_flag << " ";
	  os << " 0 0"
	     << " \"" << MyName << "\"" << endl;

	}
      
      break;

    case STATUSFORMAT_NORMAL:
      if ( DAQ_RUNNING ) 
	{
	  os << MyHostName << "Run " << TheRun  
	     << " Event: " << Event_number -1 
	     << " Volume: " << volume;
	}
      else
	{
	  os << MyHostName << "Stopped";
	}
      
      if ( daq_open_flag )
	{
	  if ( daq_server_flag )
	    {
	      os << "  Logging enabled via remote server " << daq_server_name << " Port " << daq_server_port;
	    }
	  else
	    {
	      os << "  Logging enabled";
	      //if ( daqBufferVector[0]->getCompression() ) os << " compression enabled" << endl;
	      //if ( daqBufferVector[0]->getMD5Enabled() ==0 ) os << " MD5 calculation disabled" << endl;
	    }
	}
      else
	{
	  os << "  Logging disabled";
	}
    
      // if ( RolloverLimit)
      // 	{
      // 	  os << "  File rollover: " << RolloverLimit << "GB";
      // 	}
      
      os<< endl;
      
      break;

    default:  // flag 2++
      
      if ( DAQ_RUNNING ) 
	{
	  os << MyHostName << " running"  << endl;
	  //os << " " << MyHostName << ":" << endl;
	  //os << "  Running" << endl;
	  os << "  Run Number:    " << TheRun  << endl;
	  os << "  Event:         " << Event_number << endl;;
	  os << "  Run Volume:    " << volume << " MB"<< endl;
	  os << "  Filerule:      " <<  daq_get_filerule() << endl;
	  if ( RolloverLimit)
	    {
	      os << "  File rollover: " << RolloverLimit << "GB" << endl;
	    }
	  //else
	  //  {
	  //    os << "  File rollover:    disabled" << endl;
	  //  }

	  if ( daq_open_flag )
	    {
	      if ( daq_server_flag )
		{
		  os << "  Filename on server: " << get_current_filename() << endl;
		}
	      else
		{
		  os << "  Filename:      " << get_current_filename() << endl; 	  
		  os << "  Number of buffers/write threads: " << nr_of_write_threads << endl;
		  os << "  Number of buffers/write threads: " << nr_of_write_threads << " Buffersize: " << daqBufferVector[0]->getMemSize()/256/1024 << "MB" << endl;
		  if ( daqBufferVector[0]->getCompression() ) os << "  compression enabled" << endl;
		  else  os << "  compression disabled" << " level: " << daqBufferVector[0]->getCompression() <<endl;
		  if ( daqBufferVector[0]->getMD5Enabled() ==0 ) os << "  MD5 calculation disabled" << endl;
		  else os << "  MD5 calculation enabled" << endl;

		}
	    }

	  os << "  Duration:      " <<  time(0) - StartTime << " s" <<endl;

	  if (max_volume)
	    {
	      os << "  Volume Limit: " <<  max_volume /(1024 *1024) << " Mb" << endl;
	    }
	  if (max_events)
	    {
	      os << "  Event Limit:  " <<  max_events << endl;
	    }
	}
      else  // not runnig
	{
	  os << MyHostName << " Stopped"  << endl;
	  os << "  Filerule:     " <<  daq_get_filerule() << endl;
	  if ( daq_open_flag )
	    {
	      if ( daq_server_flag )
		{
		  os << "  Logging enabled via remote server " << daq_server_name << " Port " << daq_server_port << endl;
		}
	      else
		{
		  os << "  Logging enabled" << endl;
		  os << "  Number of buffers/write threads: " << nr_of_write_threads << " Buffersize: " << daqBufferVector[0]->getMemSize()/256/1024 << "MB" << endl;
		  if ( daqBufferVector[0]->getCompression() ) os << "  compression enabled" << endl;
		  else  os << "  compression disabled" << " level: " << daqBufferVector[0]->getCompression() <<endl;
		  if ( daqBufferVector[0]->getMD5Enabled() ==0 ) os << "  MD5 calculation disabled" << endl;
		  else os << "  MD5 calculation enabled" << endl;
		}
	    }
	  else
	    {
	      if ( daq_server_flag )
		{
		  os << "  Logging disabled, remote server set " << daq_server_name << " Port " << daq_server_port << endl;
		}
	      else
		{
		  os << "  Logging disabled" << endl;
		}
	    }	      
	  
	  if ( RolloverLimit)
	    {
	      os << "  File rollover: " << RolloverLimit << "GB" << endl;
	    }
	  //else
	  //  {
	  //    os << "  File rollover:    disabled" << endl;
	  //  }

	  
	  if (max_volume)
	    {
	      os << "  Volume Limit: " <<  max_volume /(1024 *1024) << " Mb" << endl;
	    }
	  if (max_events)
	    {
	      os << "  Event Limit:  " <<  max_events << endl;
	    }
	  if ( TriggerH ) os << "  have a trigger object" << endl;
	}
      if (RunControlMode)
	{
	  os << "  Run Control Mode enabled  " << endl;
	}

      os << "  Buffer Sizes:     " <<  daqBufferVector[0]->getMaxSize()/1024 << " KB";
      if ( adaptivebuffering) 
	{
	  os << " adaptive buffering: " << adaptivebuffering << " s"; 
	}
      os << endl;

      if ( ThePort)
      	{
      	  os << "  Web control Port:  " << ThePort <<  endl;
      	}
      else
      	{
      	  os << "  No Web control defined" << endl;
      	}

      if ( daq_getEventFormat())
	{
	  os << "  Writing legacy format " << endl;
	}

      if ( ElogH)
      	{
      	  os << "  Elog:  " << ElogH->getHost() << " " << ElogH->getLogbookName() << " Port " << ElogH->getPort() <<  endl;
      	}
      else
      	{
      	  os << "  Elog: not defined" << endl;
      	}
#ifdef HAVE_MOSQUITTO_H
      if (mqtt)
	{
	  os << "  mqtt:     " << mqtt->GetHostName() << " Port " << mqtt->GetPort() << endl;  
	}
#endif
      
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
      ThePort=8899 + servernumber;
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
      os << MyHostName << "error in web service creation " << status << endl;
      ThePort=0;
      return -1;
    }
  else
    {
      os << MyHostName << "web service created" << endl;
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

int daq_getlastevent_number( std::ostream& os)
{
  os <<  Event_number_at_last_end << endl;
  return Event_number_at_last_end;
}

int daq_running()
{
  if ( DAQ_RUNNING ) return 1;
  return 0;
}


// the routines to deal with the remote server if we are logging this way.

// 1) we open a socket with            open_serverSocket
// 2) make the server open a file with server_send_beginrun_sequence
// 3) ... send buffers
// 4) send end-run with                server_send_endrun_sequence
// 5) rinse and repeat 2...4
// 6) tell the server we are done with server_send_close_sequence 

int open_serverSocket(const char * hostname, const int port)
{

  //  extern int h_errno;
  int sockfd = 0;

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  
  hints.ai_family = AF_INET;
  struct addrinfo *result, *rp;

  char port_str[512];
  sprintf(port_str, "%d", port);

  
  int status = getaddrinfo(hostname, port_str,
                       &hints,
                       &result);

  if ( status < 0)
    {
      cout << __FILE__<< " " << __LINE__ << " " << hostname << " " << gai_strerror(status) << endl;
      return status;
    }
  
  for (rp = result; rp != NULL; rp = rp->ai_next)
    {

      if ( (sockfd = socket(result->ai_family, result->ai_socktype,
                            result->ai_protocol) ) > 0 )
	{
	  break;
	}
    }

  if ( sockfd < 0)
      {
	cerrfl << " error in socket" << std::endl;
	perror("socket");
	freeaddrinfo(result);
	return -1;
      }
  
      int xs = 512*1024;
  
      int s = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &xs, sizeof(int));
      if (s) std::cout << "setsockopt status = " << s << std::endl;

  if ( connect(sockfd, rp->ai_addr, rp->ai_addrlen) < 0 ) 
    {
      cerrfl << " error in connect" << std::endl;
      perror("connect");
      freeaddrinfo(result);
      return -1;
    }

  freeaddrinfo(result);
  return sockfd;
}

int server_send_beginrun_sequence(const char * filename, const int runnumber, int fd)
{
  int opcode;
  int status;
  int len;
  
  // std::cout << __FILE__ << " " << __LINE__ << " sending    " << CTRL_SENDFILENAME << std::endl ;
  opcode = htonl(CTRL_SENDFILENAME);
  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;

  len = strlen(filename);
  opcode = htonl(len);
  //std::cout << __FILE__ << " " << __LINE__ << " sending    " << filename  << " len = " << len<< std::endl ;

  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;

  status = writen(fd, (char *) filename, len);
  if ( status != len) return -1;

  status = readn (fd, (char *) &opcode, sizeof(int) );
  if ( status != sizeof(int)  || ntohl(opcode) != CTRL_REMOTESUCCESS )
    {
      perror("read_ack");
      return -1;
    }

  //std::cout << __FILE__ << " " << __LINE__ << " sending    " << CTRL_BEGINRUN  << std::endl ;
  opcode = htonl(CTRL_BEGINRUN);
  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;

  //std::cout << __FILE__ << " " << __LINE__ << " sending    " << runnumber  << std::endl ;
  opcode = htonl(runnumber);
  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;
  
  //std::cout << __FILE__ << " " << __LINE__ << " waiting for acknowledge...   " << std::endl ;
  status = readn (fd, (char *) &opcode, sizeof(int) );
  //std::cout << __FILE__ << " " << __LINE__ << " returned status " <<  ntohl(opcode) << endl;
  if ( status != sizeof(int)  )
    {
      perror("read_ack");
      return -1;
    }
  if (ntohl(opcode) != CTRL_REMOTESUCCESS )
    {
      return -1;
    }

  return 0;
}

int server_send_rollover_sequence(const char * filename, int fd)
{
  int opcode;
  int status;
  int len;
  
  // std::cout << __FILE__ << " " << __LINE__ << " sending    " << CTRL_SENDFILENAME << std::endl ;
  opcode = htonl(CTRL_ROLLOVER);
  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;

  len = strlen(filename);
  opcode = htonl(len);
  //std::cout << __FILE__ << " " << __LINE__ << " sending    " << filename  << " len = " << len<< std::endl ;
  status = writen(fd, (char *) &opcode, sizeof(int));
  if ( status != sizeof(int)) return -1;

  status = writen(fd, (char *) filename, len);
  if ( status != len) return -1;

  status = readn (fd, (char *) &opcode, sizeof(int) );
  if ( status != sizeof(int)  || ntohl(opcode) != CTRL_REMOTESUCCESS )
    {
      perror("read_ack");
      return -1;
    }

  return 0;
}


int server_send_endrun_sequence(int fd)
{
  int opcode;
  int status;

  opcode = htonl(CTRL_ENDRUN);
  status = writen (fd, (char *)&opcode, sizeof(int) );
  if ( status != sizeof(int)) return -1;
  
  //  std::cout << __FILE__ << " " << __LINE__ << " waiting for acknowledge...   " << std::endl ;
  status = readn (fd, (char *) &opcode, sizeof(int) );
  if ( status != sizeof(int)  || ntohl(opcode) != CTRL_REMOTESUCCESS )
    {
      perror("read_ack");
      return -1;
    }
  //std::cout << __FILE__ << " " << __LINE__ << " ok " << std::endl;

  return 0;
}

int server_send_close_sequence(int fd)
{
  int opcode;
  int status;

  opcode = htonl(CTRL_CLOSE);
  status = writen (fd, (char *)&opcode, sizeof(int) );
  if ( status != sizeof(int)) return -1;
  
  return 0;
}


int daq_generate_json (const int flag)
{
#ifdef HAVE_MOSQUITTO_H

  if ( !  mqtt) return 0;
  
  std::ostringstream out;

  if (flag == 0) // we start a new entry
    {

      out << "{\"file\": [" << endl;
      out << "    { \"what\":\"new\","
	  << " \"runnumber\":" << TheRun << ","
	  << " \"host\":\"" << shortHostName << "\","
	  << " \"runtype\":\"" << TheRunType << "\","
	  << " \"CurrentFileName\":\"" << CurrentFilename << "\","
	  << " \"CurrentFileSequence\":" << current_filesequence << ","
	  << " \"FirstEventNr\":" << Event_number_at_last_open << ","
	  << " \"time\": " << time(0)  << " }" << endl;
      out << "] }" << endl;
    }
  else   // update an entry
    {
      md5_byte_t md5_digest[16];  
      char digest_string[33];

      if ( md5_enabled) 
	{
	  md5_finish(&md5state, md5_digest);
	  for ( int i=0; i< 16; i++) 
	    {
	      sprintf ( &digest_string[2*i], "%02x",  md5_digest[i]);
 	    }
	  digest_string[32] = 0;
	}
      else
	{
	  for ( int i=0; i< 16; i++) 
	    {
	      sprintf ( &digest_string[2*i], "ff");
 	    }
	  digest_string[32] = 0;
	}

      out << "{\"file\": [" << endl;
      out << "    { \"what\":\"" << "update"
	  << "\", \"runnumber\":" << TheRun << ","
	  << " \"host\":\"" << shortHostName << "\","
	  << " \"CurrentFileName\":\"" << CurrentFilename << "\","
	  << " \"MD5\":\"" << digest_string << "\","
	  << " \"LastEventNr\":" << daqBufferVector[last_written_buffernr]->getLastEventNumber() << ","
	  << " \"NrEvents\":" << daqBufferVector[last_written_buffernr]->getLastEventNumber() - Event_number_at_last_open +1 << ","
	  << " \"time\":" << time(0)  << " }" << endl;
      out << "] }" << endl;
    }

  mqtt->send(out.str());
  
#endif
  
  return 0;
}

int daq_set_uservalue ( const int index, const int value, std::ostream& os  )
{
  if ( index < 0 || index > 7)
    {
      os << "index out of range. (0..7)" << endl;
      return -1;
    }
  uservalues[index] = value;
  return 0;
}


int daq_get_uservalue ( const int index,  std::ostream& os )
{
  if ( index < 0 || index > 7)
    {
      os << "index out of range. (0..7)" << endl;
      return -1;
    }
  os << uservalues[index] << endl;
  return 0;

}

int get_uservalue ( const int index)
{
  if ( index < 0 || index > 7)
    {
      return 0;
    }
  return uservalues[index];

}

