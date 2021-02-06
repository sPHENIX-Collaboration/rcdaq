
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/ipc.h>

#include <pthread.h>

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#include <sys/time.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h> 

#include <sys/wait.h>
#include <sys/resource.h>


#include <sys/types.h>

#ifdef SunOS
#include <sys/filio.h>
#endif

#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <iostream>
#include <iomanip>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif


typedef unsigned int PHDWORD;
typedef unsigned short SWORD;
typedef unsigned char BYTE;
typedef unsigned int UINT;

#define BUFFERBLOCKSIZE 8192U


#define CTRL_BEGINRUN        1
#define CTRL_ENDRUN          2
#define CTRL_DATA            3
#define CTRL_CLOSE           4
#define CTRL_SENDFILENAME    5

#define CTRL_REMOTESUCCESS 100
#define CTRL_REMOTEFAIL    101

#define ROLE_RECEIVED 0
#define ROLE_WRITTEN  1



// initial size 16 Mbytes ( /4 in int units)
#define INITIAL_SIZE (4*1024*1024)


typedef struct 
{
  int dirty;
  int role;
  int bytecount;
  int buffersize;
  PHDWORD *bf;
} bufferstructure;

int run_number = 0;
int verbose = 0;
int sockfd = 0;
int dd_fd = 0;


pthread_mutex_t M_cout;

pthread_mutex_t M_write;
pthread_mutex_t M_done;


pthread_t ThreadId;
pthread_t       tid;
int output_fd = -1;

int the_port = 5001;

int RunIsActive = 0; 
int NumberWritten = 0; 
int file_open = 0;


int readn(int , char *, int);
int writen(int , char *, int);

#if defined(SunOS) || defined(Linux) 
void sig_handler(int);
#else
void sig_handler(...);
#endif

void *writebuffers ( void * arg);
int handle_this_child( pid_t pid);
in_addr_t find_address_from_interface(const char *);

void cleanup(const int exitstatus);


bufferstructure *bf_being_received;
bufferstructure *bf_being_written;
 

using namespace std;


void exitmsg()
{
  cout << "** This is the Advanced Multithreaded Server. No gimmicks. Pure Power." << endl;
  cout << "** usage: sfs  port-number" << endl;
  cout << "   -d enable database logging" << endl;
  cout << "   -b interface    bind to this interface" << endl;
  exit(0);
}


//char *s_opcode[] = {
//		    "Invalid code",
//		    "CTRL_BEGINRUN",
//		    "CTRL_ENDRUN",
//		    "CTRL_DATA",
//		    "CTRL_CLOSE",
//		    "CTRL_SENDFILENAME"};

std::string listen_interface;

int main( int argc, char* argv[])
{

  int status;


#if defined(SunOS) || defined(Linux) 
  struct sockaddr client_addr;
#else
  struct sockaddr_in client_addr;
#endif
  struct sockaddr_in server_addr;

#if defined(SunOS)
  int client_addr_len = sizeof(client_addr);
#else
  unsigned int client_addr_len = sizeof(client_addr);
#endif

  pthread_mutex_init(&M_cout, 0); 
  pthread_mutex_init(&M_write, 0); 
  pthread_mutex_init(&M_done, 0); 

  pthread_mutex_lock(&M_write);

  // by default, we listen on all interfaces
  in_addr_t listen_address = htonl(INADDR_ANY);  

  char c;
  
  while ((c = getopt(argc, argv, "vdb:")) != EOF)
    {
      switch (c) 
	{

	case 'v':   // verbose
	  verbose += 1;
	  break;

	case 'b':   // bind to this interface
	  listen_address = find_address_from_interface(optarg);
	  listen_interface = optarg;
	  break;

	case 'd':   // no database
	  // databaseflag=1;
	  // cout << "database access enabled" << endl;
	  break;


	}
    }



  if (argc > optind)
    {
      sscanf (argv[optind],"%d", &the_port);
    }
  
  // ------------------------
  // now set up the sockets

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
      cleanup(1);
    }
  //  int xs = 64*1024+21845;
  int xs = 1024*1024;

  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		     &xs, 4);


  memset( &server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = listen_address;
  server_addr.sin_port = htons(the_port);

  int i;
  
  if ( ( i = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0 )
    {
      perror(" bind ");
      cleanup(1);
    }

  if ( ( i =  listen(sockfd, 100) ) )
    {
      perror(" listen ");
      cleanup(1);
    }

  if (verbose)
    {
      cout << " listening on port " << the_port;
      if (! listen_interface.empty() ) cout << " on interface " << listen_interface;
      cout << endl; 
    }
  
  signal(SIGCHLD, SIG_IGN); 

  pid_t pid;
  struct sockaddr_in out;
  
  while  (sockfd > 0)
    {
			
      client_addr_len = sizeof(out); 
      dd_fd = accept(sockfd,  (struct sockaddr *) &out, &client_addr_len);
      if ( dd_fd < 0 ) 
	{
	  pthread_mutex_lock(&M_cout);
	  cout  << "error in accept socket" << endl;
	  pthread_mutex_unlock(&M_cout);
	  perror (" accept" );
	  cleanup(1);
	  exit(1);
	}


      if (verbose)
	{
	  char *host = new char[512]; 
	  getnameinfo((struct sockaddr *) &out, sizeof(struct sockaddr_in), host, 511,
		      NULL, 0, NI_NOFQDN); 
    	  cout << " new connection from " << host << endl;
	}
      
      
      if ( (pid = fork()) == 0 ) 
	{
	  handle_this_child( pid);
	}
    }
}

int handle_this_child( pid_t pid)
{


  int controlword;
  int len;
  int local_runnr = 0;

  bufferstructure B0;
  bufferstructure B1;
  
  bf_being_received = &B0;
  bf_being_written = &B1;
  bufferstructure *bf_temp;

  B0.bf = new PHDWORD[INITIAL_SIZE];
  B0.buffersize = INITIAL_SIZE;
  B0.dirty = 0;
  B0.role= ROLE_RECEIVED;
  B0.bytecount = 0;
		      
  B1.bf = new PHDWORD[INITIAL_SIZE];
  B1.buffersize = INITIAL_SIZE;
  B1.dirty = 0;
  B1.role= ROLE_RECEIVED;
  B1.bytecount = 0;
		      
  int i;
  
  // we make a thread tht will write out our buffers
  i = pthread_create(&ThreadId, NULL, 
		     writebuffers, 
		     (void *) 0);
  if (i )
    {
      cout << "error in thread create " << i << endl;
      perror("Thread ");
      cleanup(1);
    }
  // else
  //   {
  //     pthread_mutex_lock(&M_cout); 
  //     cout << "write thread created" << endl;
  //     pthread_mutex_unlock(&M_cout);
  //   }
    
  // should be the default, but set it to blocking mode anyway
  i = ioctl (dd_fd,  FIONBIO, 0);
    
  // find out where we were contacted from
    

  int status;
  int xx;

  int go_on = 1;
  while ( go_on)
    {
      if ( (status = readn (dd_fd, (char *) &xx, 4) ) <= 0)
	{
	  cout  << "error in read from socket" << endl;
	  perror ("read " );
	  cleanup(1);
	  exit(1);
	}
	
      controlword = ntohl(xx);

      // cout << endl;
      // cout << __FILE__ << " " << __LINE__  << " controlword = " << controlword << " ntew: " << xx << " " ;
      // if ( controlword >=0 && controlword <=5) cout  << s_opcode[controlword];
      // cout << endl;
	
      char *p;
      char filename[1024];
      int value, len;
      
      switch (controlword) 
	{

	case  CTRL_SENDFILENAME:
	  // read the length of what we are about to get 
	  readn (dd_fd, (char *) &len, sizeof(int));
	  len  = ntohl(len);
	  // acknowledge... or not
	  //cout  << " filename len  = " << len << endl;
	  if ( len >= 1023)
	    {
	      i = htonl(CTRL_REMOTEFAIL);
	      writen (dd_fd, (char *)&i, 4);
	      break;
	    }
	  value = readn (dd_fd, filename, len);
	  filename[value] = 0;
	  //cout  << " filename is " << filename << endl;

          i = htonl(CTRL_REMOTESUCCESS);
          writen (dd_fd, (char *)&i, 4);
	  
	  break;
	  
	case  CTRL_BEGINRUN:
	  
	  readn (dd_fd, (char *) &local_runnr, 4);
	  local_runnr = ntohl(local_runnr);
	  //cout  << " runnumber = " << local_runnr << endl;

	  output_fd =  open(filename,  O_WRONLY | O_CREAT | O_TRUNC | O_EXCL | O_LARGEFILE , 
			    S_IRWXU | S_IROTH | S_IRGRP );
	  if (output_fd < 0) 
	    {
	      cerr << "file " << filename << " exists, I will not overwrite " << endl;
	      i = htonl(CTRL_REMOTEFAIL);
	      writen (dd_fd, (char *)&i, 4);
	      break;
	    }
	  if (verbose)
	    {
	      cout << " opened new file " << filename << endl; 
	    }
    
	  bf_being_received = &B0;
	  bf_being_written = &B1;
	    
	  bf_being_received->role = ROLE_RECEIVED;
	  bf_being_received->dirty=0;
	    
	  bf_being_written->role = ROLE_WRITTEN;
	  bf_being_written->dirty = 0;
	    
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, 4);
	  
	  break;
	  
	case  CTRL_ENDRUN:
	  //cout  << " endrun signal " << endl;
	  
	  pthread_mutex_lock(&M_done);
	  close (output_fd);
	  if (verbose)
	    {
	      cout << " closed file "  << filename << endl; 
	    }

	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, 4);
	  pthread_mutex_unlock(&M_done);

	  break;
	  
	case  CTRL_DATA:
	  status = readn (dd_fd, (char *) &len, 4);
	  len = ntohl(len);
	  //cout  << " data! len = " << len << endl;
	  
	  bf_being_received->bytecount = len;
	  if ( (len+3)/4 > bf_being_received->buffersize)
	    { 
	      delete [] bf_being_received->bf;
	      bf_being_received->buffersize = (len+3)/4 + 2048;
	      
	      pthread_mutex_lock(&M_cout);
	      //	      cout << "expanding buffer to " << bf_being_received->buffersize << " for host " << host << endl;
	      pthread_mutex_unlock(&M_cout);
	      
	      bf_being_received->bf = new PHDWORD[ bf_being_received->buffersize];
	    }
	  
	  p = (char *) bf_being_received->bf;
	  status = readn (dd_fd, p,   len );
	  if ( len != status)
	    {
	      
	      pthread_mutex_lock(&M_cout);
	      //	      cout << "error on transfer: Expected " << len 
	      //	   << " got status  " << status  << "  for host " << host << endl;
	      perror("ctrl_data");
	      pthread_mutex_unlock(&M_cout);
	      i = htonl(CTRL_REMOTEFAIL);
	    }
	  // store the actual byte length we received
	  bf_being_received->bytecount = len;
	  bf_being_received->dirty = 1;
 
	  //  cout << __FILE__ << " " << __LINE__ << " waiting for write thread " << endl;
	  pthread_mutex_lock(&M_done);
	  // and switch the buffers
	  bf_temp = bf_being_received;
	  bf_being_received = bf_being_written;
	  bf_being_written = bf_temp;
	  bf_being_written->role = ROLE_WRITTEN;
	  bf_being_received->role = ROLE_RECEIVED;
	      
	  //	  cout << __FILE__ << " " << __LINE__ << " switching buffers " << bf_being_received << "  " << bf_being_received << endl;
	  pthread_mutex_unlock(&M_write);
	  
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, sizeof(int));
	  
	  break;

	case CTRL_CLOSE:
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, sizeof(int));
	  close ( dd_fd);
	  go_on = 0;
	  break;
	  
	}
		  
    }
  
  return 0;
}



void *writebuffers ( void * arg)
{

  while(1)
    {

      pthread_mutex_lock(&M_write);

      pthread_mutex_lock(&M_cout);
      //cout << __LINE__ << "  " << __FILE__ << " write thread unlocked  " <<  endl;
      pthread_mutex_unlock(&M_cout);

      int blockcount = ( bf_being_written->bytecount + BUFFERBLOCKSIZE -1)/BUFFERBLOCKSIZE;
      int bytecount = blockcount*BUFFERBLOCKSIZE;
      
      pthread_mutex_lock(&M_cout);
      //cout << __LINE__ << "  " << __FILE__ << " write thread unlocked, block count " << blockcount <<  endl;
      pthread_mutex_unlock(&M_cout);

      int bytes = writen ( output_fd, (char *) bf_being_written->bf , bytecount );
      if ( bytes != bytecount)
	{
	  pthread_mutex_lock(&M_cout);
	  cout << __LINE__ << "  " << __FILE__ << " write error " << bytes << "  " << bytecount <<  endl;
	  pthread_mutex_unlock(&M_cout);
	  bf_being_written->dirty = -1;  // mark as "error"
	}
      else
	{
	  //      usleep(1000000);
	  bf_being_written->dirty = 0;
	}
      //      bytes = writen ( fd, (char *) buffers[i].bf , bytecount );
      pthread_mutex_unlock(&M_done);
      
    }
  return 0;
}




int readn (int fd, char *ptr, int nbytes)
{

  int nread, nleft;
  //int nleft, nread;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nread = read (fd, ptr, nleft);
      if ( nread <= 0 ) 
	{
	  return nread;
	}

#ifdef FRAGMENTMONITORING
      history[hpos++] = nread;
#endif
      nleft -= nread;
      ptr += nread;
    }

#ifdef FRAGMENTMONITORING
  if ( hpos >1 ) 
    {
      cout << "Fragmented transfer of " << nbytes << "bytes: ";
      for ( int i=0; i<hpos; i++)
	{
	  cout << " " << history[i]<< ",";
	}
      cout << endl;
    }
#endif
  return (nbytes-nleft);
}
  
int writen (int fd, char *ptr, int nbytes)
{
  int nleft, nwritten;
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


in_addr_t find_address_from_interface(const char *interface)
{
  int fd;
  struct ifreq ifr;

  // check the no one plays tricks with us...
  if ( strlen(interface) >= IFNAMSIZ)
    {
      cerr << " Interface name too long, ignoring "<< endl;
      return htonl(INADDR_ANY);
    }
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  // I want to find an IPv4 IP address
  ifr.ifr_addr.sa_family = AF_INET;

  // I want IP address attached to "interface"
  strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

  ioctl(fd, SIOCGIFADDR, &ifr);

  close(fd);
  in_addr_t a =  ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

  if (verbose)  cout << "  binding to address " << inet_ntoa(  ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr ) << endl;
  return a;
}



void cleanup( const int exitstatus)
{
  close (sockfd);
  close (dd_fd);

  exit(exitstatus);
}
    
