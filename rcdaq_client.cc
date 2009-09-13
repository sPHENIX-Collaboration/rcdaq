


#include <iostream>
#include <iomanip>
#include <getopt.h>

#include <sstream>
#include "rcdaq_rpc.h"
#include "rcdaq_actions.h"

using namespace std;

CLIENT *clnt;

//result *ra;
int  verbose_flag = 0;

void
rcdaq_client_Init(char *host, const int servernumber)
{


  clnt = clnt_create (host, RCDAQ+servernumber, RCDAQ_VERS, "tcp");
  if (clnt == NULL) {
    clnt_pcreateerror (host);
    exit (1);
  }
  
  struct timeval tv;
  tv.tv_sec = 1000;
  tv.tv_usec = 0;
  
  clnt_control(clnt,CLSET_TIMEOUT , (char *) &tv );
  
  
  
}

void showHelp()
{
  std::cout << "  help                 show this help text"  << std::endl; 
  std::cout << std::endl; 
  std::cout << "  daq_begin            start taking data"  << std::endl; 
  exit(0);
}



int handle_device( int argc, char **argv, const int optind)
{

  struct shortResult  *r;
  struct actionblock ab;
  ab.spar = " ";
  ab.spar2 = " ";

  ab.action = DAQ_DEVICE;

  if ( strcasecmp(argv[optind+1],"device_random") == 0 ) 
    {

      if ( argc < optind + 4) return -1;

      ab.spare = DAQ_DEVICE_RANDOM;

      ab.ipar[0] = atoi ( argv[optind+2]); // event type
      ab.ipar[1] = atoi ( argv[optind+3]); // subevent id
      ab.ipar[15] = 2; // how many params so far

      if (  argc > optind + 4)
	{
	  ab.ipar[2] =  atoi ( argv[optind+4]); // how many words
	  ab.ipar[15]++;
	  
	}
      if (  argc > optind + 5)
	{
	  ab.ipar[3] =  atoi ( argv[optind+5]); // start
	  ab.ipar[15]++;
	  
	}
      if (  argc > optind + 6)
	{
	  ab.ipar[4] =  atoi ( argv[optind+6]); // end
	  ab.ipar[15]++;
	  
	}

    }
  r = r_create_device_1(&ab, clnt);
  if (r == (shortResult *) NULL) 
    {
      clnt_perror (clnt, "call failed");
    }
  if ( r->content) std::cout <<  r->str << std::endl;
  
  return 0;
}


int command_execute( int argc, char **argv)
{
  struct shortResult  *r;
  struct actionblock  ab;

  if ( argc <= 1 ) showHelp();
  
  char tempSelection[1024];
  
  char command[256];
  
  
  int c;
  
  optind = 0; // tell getopt to start over
  int servernumber=0;

  while ((c = getopt(argc, argv, "v")) != EOF)
    {
      switch (c) 
	{

	case 'v': // set the verbose flag
	  verbose_flag = 1;
	  break;

	  //	case 's': // determine the server number
	  //	  servernumber=atoi(argv[optind+1]);
	  //	  break;

	}
    }
  //  std::cout << "Server number is " << servernumber << std::endl;

  char *host = "localhost";
    
  if ( getenv("RCDAQHOST")  )
    {
      host = getenv("RCDAQHOST");
    }
  
  rcdaq_client_Init (host,servernumber);
  
  ab.action = 0;
  ab.value  = 0;
  ab.spare = 0;
  
  int i;
  for ( i = 0; i< 16; i++)
    {
      ab.ipar[i] = 0;
    }
  
  ab.spar = " ";

  strcpy(command, argv[optind]);


  if ( strcasecmp(command,"help") == 0 ) 
    {
      showHelp();
    }

  else if ( strcasecmp(command,"daq_begin") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_BEGIN;
      ab.ipar[0] = atoi(argv[optind + 1]);
      r = r_action_1 (&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  

  else if ( strcasecmp(command,"daq_end") == 0)
    {

      ab.action = DAQ_END;
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  
  else if ( strcasecmp(command,"daq_setfilerule") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_SETFILERULE;
      ab.spar = argv[optind + 1];
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  
  else if ( strcasecmp(command,"daq_open") == 0)
    {

      ab.action = DAQ_OPEN;
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  
  else if ( strcasecmp(command,"daq_close") == 0)
    {

      ab.action = DAQ_CLOSE;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  
  else if ( strcasecmp(command,"daq_fake_trigger") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_FAKETRIGGER;
      ab.ipar[0] = atoi(argv[optind + 1]);
      // see if we have a 2nd parameter
      if ( argc > optind + 2)
	{
	  ab.ipar[1] =  atoi(argv[optind + 1]);
	}

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }

  else if ( strcasecmp(command,"daq_list_readlist") == 0)
    {

      ab.action = DAQ_LISTREADLIST;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  

  else if ( strcasecmp(command,"elog") == 0)
    {

      if ( argc != optind + 4) return -1;

      ab.action = DAQ_ELOG;
      ab.spar = argv[optind + 1];
      ab.ipar[0] = atoi(argv[optind + 2]);
      ab.spar2 = argv[optind +3];

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::endl;
 
    }
  
  else if ( strcasecmp(command,"create_device") == 0)
    {


      return handle_device ( argc, argv, optind);
 
    }

  else
    {
      std::cout << "Unknown Command " << command << std::endl;
    }

  return 0;
}


int
main (int argc, char *argv[])
{

  int status = command_execute( argc, argv);
  if ( status)
    {
      cout << "Wrong number of parameters" << endl;
    }

  clnt_destroy (clnt);

  exit (0);
}
