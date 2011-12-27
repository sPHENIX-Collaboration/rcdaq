


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
  std::cout << "   daq_begin [run-number]      	start taking data for run-number, or auto-increment" << std::endl;
  std::cout << "   daq_end   		 	end the run " << std::endl;
  std::cout << "   daq_setfilerule file-rule 	set the file rule default rcdaq-%08d-%04d.evt" << std::endl;
  std::cout << "   daq_open  	 		enable logging" << std::endl;
  std::cout << "   daq_close  			disable logging" << std::endl;
  std::cout << "   daq_fake_trigger n 		create n artificial triggers" << std::endl;
  std::cout << "   daq_list_readlist  		display the current readout list" << std::endl;
  std::cout << "   daq_clear_readlist  		clear the current readout list " << std::endl;
  std::cout << "   daq_status [-s] [-l] 	display status [short] [long]" << std::endl;
  std::cout << "   daq_set_maxevents nevt 	set automatic end at so many events" << std::endl;
  std::cout << "   daq_set_maxvolume n_MB 	set automatic end at n_MB MegaByte" << std::endl;
  std::cout << "   daq_set_maxbuffersize n_KB 	adjust the size of buffers written to n KB" << std::endl;
  std::cout << "   elog elog-server port	specify coordinates for an Elog server" << std::endl;
  std::cout << "   create_device [device-specific parameters] " << std::endl;
  std::cout << "   daq_shutdown  		terminate the rcdaq backend" << std::endl;
  exit(0);
}



int handle_device( int argc, char **argv, const int optind)
{

  struct shortResult  *r;
  struct actionblock ab;
  ab.value=0;
  ab.spare=0;
  ab.spar = " ";
  ab.spar2 = " ";
  int i;
  for ( i = 0; i< 16; i++)
    {
      ab.ipar[i] = 0;
    }
  

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
      if (  argc > optind + 7)
	{
	  ab.ipar[5] =  1;  //say that we are trigger-enabled
	  ab.ipar[15]++;
	  
	}

    }
  else if ( strcasecmp(argv[optind+1],"device_file") == 0 ) 
    {

      if ( argc < optind + 4) return -1;

      ab.spare = DAQ_DEVICE_FILE;

      ab.ipar[0] = atoi ( argv[optind+2]); // event type
      ab.ipar[1] = atoi ( argv[optind+3]); // subevent id
      ab.spar = argv[optind+4]; // file

    }

  else if ( strcasecmp(argv[optind+1],"device_tspmproto") == 0 ) 
    {

      if ( argc < optind + 4) return -1;

      ab.spare = DAQ_DEVICE_TSPMPROTO;

      ab.ipar[0] = atoi ( argv[optind+2]); // event type
      ab.ipar[1] = atoi ( argv[optind+3]); // subevent id
      ab.spar = argv[optind+4]; // ip

    }

  else if ( strcasecmp(argv[optind+1],"device_tspmparams") == 0 ) 
    {

      if ( argc < optind + 4) return -1;

      ab.spare = DAQ_DEVICE_TSPMPARAMS;

      ab.ipar[0] = atoi ( argv[optind+2]); // event type
      ab.ipar[1] = atoi ( argv[optind+3]); // subevent id
      ab.spar = argv[optind+4]; // ip

    }

  r = r_create_device_1(&ab, clnt);
  if (r == (shortResult *) NULL) 
    {
      clnt_perror (clnt, "call failed");
    }
  if ( r->content) std::cout <<  r->str << std::flush;
  
  return r->status;
}


int command_execute( int argc, char **argv)
{
  struct shortResult  *r;
  struct actionblock  ab;

  if ( argc <= 1 ) showHelp();
  
  char tempSelection[1024];
  
  char command[256];
  
  // the log and short flags are for qualifying the 
  // status output
  int long_flag = 0;
  int short_flag = 0;

  int c;
  
  optind = 0; // tell getopt to start over
  int servernumber=0;

  while ((c = getopt(argc, argv, "vls")) != EOF)
    {
      switch (c) 
	{

	case 'v': // set the verbose flag
	  verbose_flag = 1;
	  break;

	case 'l': // set the long flag
	  long_flag = 1;
	  break;

	case 's': // set the verbose flag
	  short_flag = 1;
	  break;

	}
    }

  // long takes precedence over short
  if (  long_flag ) short_flag = 0;

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

      ab.action = DAQ_BEGIN;
      if ( argc == optind + 2)
	{
	  ab.ipar[0] = atoi(argv[optind + 1]);
	}
      else
	{
	  ab.ipar[0] = 0;
	}
      r = r_action_1 (&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  

  else if ( strcasecmp(command,"daq_end") == 0)
    {

      ab.action = DAQ_END;
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
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
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"daq_open") == 0)
    {

      ab.action = DAQ_OPEN;
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"daq_close") == 0)
    {

      ab.action = DAQ_CLOSE;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
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
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_list_readlist") == 0)
    {

      ab.action = DAQ_LISTREADLIST;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
 else if ( strcasecmp(command,"daq_clear_readlist") == 0)
    {

      ab.action = DAQ_CLEARREADLIST;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  

  else if ( strcasecmp(command,"daq_status") == 0)
    {

      ab.action = DAQ_STATUS;

      if ( long_flag) ab.ipar[0] = 1;
      else if ( short_flag) ab.ipar[0] = 2;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"daq_set_maxevents") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SETMAXEVENTS;
      ab.ipar[0] = atoi(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_maxvolume") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SETMAXVOLUME;
      ab.ipar[0] = atoi(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_maxbuffersize") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SETMAXBUFFERSIZE;
      ab.ipar[0] = atoi(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
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
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"create_device") == 0)
    {


      return handle_device ( argc, argv, optind);
 
    }

  else if ( strcasecmp(command,"daq_shutdown") == 0)
    {

      r = r_shutdown_1(0,clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	  return 0;
	}
      else
	{
	  if (r->content) std::cout <<  r->str << std::flush;
	}
    }


  else
    {
      std::cout << "Unknown Command " << command << std::endl;
      return 0;
    }

  return r->status;
}


int
main (int argc, char *argv[])
{

  int status = command_execute( argc, argv);
  clnt_destroy (clnt);

  if ( status)
    {
      return 1;
    }


  return 0;
}
