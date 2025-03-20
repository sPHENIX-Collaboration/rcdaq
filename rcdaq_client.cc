

#include <iostream>
#include <iomanip>
#include <getopt.h>

#include <sstream>
#include "parseargument.h"
#include "rcdaq_rpc.h"
#include "rcdaq_actions.h"

using namespace std;

CLIENT *clnt;

//result *ra;
int  verbose_flag = 0;

void
rcdaq_client_Init(const char *host, const int servernumber)
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
  std::cout << "   help                                 show this help text"  << std::endl; 
  std::cout << "   daq_status [-s] [-l]                 display status [short] [long]" << std::endl;
  std::cout << "   daq_open                             enable logging" << std::endl;
  std::cout << "   daq_set_server hostname [port]       choose logging to a SFS server, or \"None\" to choose local file" << std::endl;
  std::cout << "   daq_begin [run-number]             	start taking data for run-number, or auto-increment" << std::endl;
  std::cout << "   daq_end                              end the run " << std::endl;
  std::cout << "   daq_close                            disable logging" << std::endl;
  std::cout << "   daq_setfilerule file-rule            set the file rule default rcdaq-%08d-%04d.evt" << std::endl;
  std::cout << std::endl; 
  std::cout << "   daq_list_readlist                    display the current readout list" << std::endl;
  std::cout << "   daq_clear_readlist                   clear the current readout list " << std::endl;
  std::cout << std::endl; 
  std::cout << "   daq_define_runtype type file-rule    define a run type, such as \"calibration\"" << std::endl;
  std::cout << "   daq_set_runtype type                 activate a predefined run type" << std::endl;
  std::cout << "   daq_get_runtype [-l]                 list the active runtype (if any)" << std::endl;
  std::cout << "   daq_list_runtypes [-s]               list defined run types" << std::endl;
  std::cout << "   daq_get_lastfilename                 return the last file name written, if any" << std::endl;
  std::cout << "   daq_get_last_event_number            return the last event number from the previous run" << std::endl;
  std::cout << std::endl; 

  std::cout << "   daq_set_maxevents nevt               set automatic end at so many events" << std::endl;
  std::cout << "   daq_set_maxvolume n_MB               set automatic end at n_MB MegaByte" << std::endl;
  std::cout << "   daq_set_rolloverlimit n_GB           set file rollover limit at n_BG GigaByte" << std::endl;
  std::cout << "   daq_set_eventformat n                dynamically switch from standard (0) to legacy header format (1)" << std::endl;
  std::cout << "   daq_set_compression n                enable (1) or disable(0) the file compression" << std::endl;
  std::cout << "   daq_set_md5enable n                  enable (1) or disable(0) the md5 calculation (default on)" << std::endl;
  std::cout << "   daq_allow_md5turnoff n               opt in to allow (1) the md5 calculation to be turned off (default no)" << std::endl;
  std::cout << "   daq_set_nr_threads n                 adjust the number of compression threads" << std::endl;
  // maybe later std::cout << "   daq_get_nr_threads                   get the number of compression threads" << std::endl;

  std::cout << std::endl; 

  std::cout << "   load  shared_library_name            load a \"plugin\" shared library" << std::endl;
  std::cout << "   create_device [device-specific parameters] " << std::endl;
  std::cout << std::endl; 

  std::cout << "   daq_setname <string>                 define an identifying string for this RCDAQ instance" << std::endl;
  std::cout << "   daq_setrunnumberfile file            define a file to maintain the current run number, \"None\" to reset" << std::endl;
  std::cout << "   daq_setrunnumberApp file             define an App that dispenses run numbers, \"None\" to reset" << std::endl;
  std::cout << std::endl;
  std::cout << "   daq_set_adaptivebuffering seconds    enable adaptive buffering at n seconds (0 = off)" << std::endl;
  std::cout << "   daq_set_maxbuffersize n_KB           adjust the used size of buffers written to n KB" << std::endl;
  std::cout << "   daq_define_buffersize n_MB           define the actual memory allocation of the buffers (daq_set_maxbuffersize just temporarily reduces the used portion " << std::endl;
  std::cout << std::endl;

  std::cout << "   daq_webcontrol <port number>         restart web controls on a new port (default 8080)" << std::endl;
  std::cout << std::endl; 
#ifdef HAVE_MOSQUITTO_H
  std::cout << "   daq_set_mqtt_host <host> [port]      set up the MQTT host/port for the DB interaction, or \"none\" to remove" << std::endl;
  std::cout << "   daq_get_mqtt_host <host> [port]      retrieve the MQTT host/port for the DB interaction" << std::endl;
#endif
  //  std::cout << "   daq_open_sqlstream <string>          open a stream to issue SQL commands" << std::endl;
  //  std::cout << "   daq_close_sqlstream                  close a SQL stream" << std::endl;
  std::cout << std::endl; 

  std::cout << "   daq_set_runcontrolmode n             switch to Run Control Mode (1=on, 0=off)" << std::endl;
  std::cout << "   daq_get_runcontrolmode               get the current Run Control Mode setting" << std::endl;
  std::cout << std::endl; 
  std::cout << "   daq_set_uservalue n val              set the user value n (0..7) to val" << std::endl;
  std::cout << "   daq_get_uservalue n                  get the user value n (0..7)" << std::endl;
  std::cout << std::endl; 

  std::cout << "   elog elog-server port                specify coordinates for an Elog server" << std::endl;
  std::cout << std::endl; 
  std::cout << "   daq_shutdown                         terminate the rcdaq backend" << std::endl;
  exit(0);
}



int handle_device( int argc, char **argv, const int optind)
{


  struct shortResult  *r;
  struct deviceblock db;


  // this construct is here because RPC doesn't support
  // 2-dim arrays (at least AFAIK). It saves a lot of if's 
  // and else's:
  char **arglist[NSTRINGPAR];


  arglist[0] = &db.argv0;
  arglist[1] = &db.argv1;
  arglist[2] = &db.argv2;
  arglist[3] = &db.argv3;
  arglist[4] = &db.argv4;
  arglist[5] = &db.argv5;
  arglist[6] = &db.argv6;
  arglist[7] = &db.argv7;
  arglist[8] = &db.argv8;
  arglist[9] = &db.argv9;
  arglist[10] = &db.argv10;
  arglist[11] = &db.argv11;
  arglist[12] = &db.argv12;
  arglist[13] = &db.argv13;

  char empty[2] = {' ',0};

  int i;
  for ( i = 0; i < NSTRINGPAR; i++)
    {
      *arglist[i] = empty;
    }
  
  db.npar=0;

  for ( i = optind+1; i < argc; i++)
    {

      // this is only the case here - the event type
      // if it's "B, D, S, or E", we replace with the begin- data- scaler- endrun event
      if ( i == optind + 2)  
	{
	  if ( *argv[i] == 'B' || *argv[i] == 'b')
	    {
		argv[i] = (char * ) "9"; // begin-run; 
	    }
	  else if ( *argv[i] == 'D' ||  *argv[i] == 'd')
	    {
	      argv[i] = (char *) "1"; // data event
	    }	
	  else if ( *argv[i] == 'E' || *argv[i] == 'e')
	    {
	      argv[i] = (char *) "12";   // end-run
	    }
	  else if ( *argv[i] == 'S' || *argv[i] == 's')
	    {
	      argv[i] = (char *) "14";  // scaler event 
	    }
	}
      
      *arglist[db.npar++] = argv[i];
      
      if ( db.npar >= NSTRINGPAR) 
	{
	  cout << "Too many parameters to handle for me" << endl;
	  break;
	}
    }
      
  r = r_create_device_1(&db, clnt);
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
  
  char command[512];
  
  // the log and short flags are for qualifying the 
  // status output
  int long_flag = 1;
  int immediate_flag = 0;  // only used for daq_end so far

  int c;
  
  optind = 0; // tell getopt to start over
  int servernumber=0;

  while ((c = getopt(argc, argv, "vlsij")) != EOF)
    {
      switch (c) 
	{

	case 'v': // set the verbose flag
	  verbose_flag++;
	  break;

	case 'l': // set the long flag; we can step up the verbosity with multiple -l's 
	  long_flag++;
	  break;

	case 's': // set the short flag (0) (note that long_flag is pre-set to 1 for "normal") 
	  long_flag=0;
	  break;

	case 'i': // set the "immediate" flag; onoy used in daq_end to request an asynchronous end
	  immediate_flag=1;
	  break;

	case 'j': // special case for daq_status to give json output
	  long_flag=100;
	  break;

	}
    }

  
  std::string rcdaqhost;
  std::string host = "localhost";
    
  if ( getenv("RCDAQHOST")  )
    {
      rcdaqhost = getenv("RCDAQHOST");
      istringstream ss(rcdaqhost);
      string token;
      if ( std::getline(ss, token, ':'))
	{
	  if ( token.size())
	    {
	      host = token;
	    }
	}
      if ( std::getline(ss, token, ':') )
	{
	  if ( token.size())
	    {
	      servernumber=std::stoi(token);
	    }
	}
    }

  
  rcdaq_client_Init (host.c_str(),servernumber);
  
  ab.action = 0;
  ab.value  = 0;
  ab.spare = 0;
  
  int i;
  for ( i = 0; i< NIPAR; i++)
    {
      ab.ipar[i] = 0;
    }

  char space[] = " ";
  ab.spar = space;
  ab.spar2 = space;

  int convertstatus;

  strcpy(command, argv[optind]);

  if ( strcasecmp(command,"help") == 0 ) 
    {
      showHelp();
    }

  else if ( strcasecmp(command,"daq_sync") == 0)
    {

      ab.action = DAQ_SYNC;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_begin") == 0)
    {

      if (immediate_flag)
	{
	  ab.action = DAQ_BEGIN_IMMEDIATE;
	}
      else
	{
	  ab.action = DAQ_BEGIN;

	}

      if ( argc == optind + 2)
	{
	  ab.ipar[0] = get_value(argv[optind + 1]);
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

      if (immediate_flag)
	{
	  ab.action = DAQ_END_IMMEDIATE;
	}
      else
	{
	  ab.action = DAQ_END;
	}
      
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
  
  else if ( strcasecmp(command,"daq_setrunnumberfile") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_RUNNUMBERFILE;
      ab.spar = argv[optind + 1];
      
      // if we set "none", unset the app
      if ( strcasecmp(argv[optind + 1],"None") == 0) ab.ipar[0] = 1;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"daq_setrunnumberApp") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_SETRUNNUMBERAPP;
      ab.spar = argv[optind + 1];

      // if we set "none", unset the app
      if ( strcasecmp(argv[optind + 1],"None") == 0) ab.ipar[0] = 1;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  // the 2nd half is not to break scripts that may use the older name 
  else if ( strcasecmp(command,"daq_set_runtype") == 0 || strcasecmp(command,"daq_setruntype") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_SETRUNTYPE;
      ab.spar = argv[optind + 1];
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_get_runtype") == 0)
    {
      
      ab.action = DAQ_GETRUNTYPE;
      ab.ipar[0] = long_flag; 

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
      
    }
  
  else if ( strcasecmp(command,"daq_define_runtype") == 0)
    {
      if ( argc != optind + 3) return -1;

      ab.action = DAQ_DEFINERUNTYPE;
      ab.spar = argv[optind + 1];
      ab.spar2 = argv[optind + 2];
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_list_runtypes") == 0)
    {
      
      ab.action = DAQ_LISTRUNTYPES;
      ab.ipar[0] = long_flag; 
      
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
      
    }
  
  else if ( strcasecmp(command,"daq_setname") == 0)
    {
      if ( argc != optind + 2) return -1;

      ab.action = DAQ_SETNAME;
      ab.spar = argv[optind + 1];
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_getname") == 0)
    {
      ab.action = DAQ_GETNAME;
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

  else if ( strcasecmp(command,"daq_set_server") == 0)
    {
      
      if ( argc <  optind + 2) return -1;

      ab.action = DAQ_SET_SERVER;
      ab.spar = argv[optind + 1];
      if ( argc == optind + 3)
	{
	  ab.ipar[0] = get_value(argv[optind + 2]);
	}
      else
	{
	  ab.ipar[0] = 0;
	}
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


  else if ( strcasecmp(command,"daq_set_compression") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_COMPRESSION;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_nr_threads") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_NR_THREADS;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_md5enable") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_MD5ENABLE;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  
  else if ( strcasecmp(command,"daq_allow_md5turnoff") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_MD5ALLOWTURNOFF;
      ab.ipar[0] = get_value(argv[optind + 1]);

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
      ab.ipar[0] = get_value(argv[optind + 1]);
      // see if we have a 2nd parameter
      if ( argc > optind + 2)
	{
	  ab.ipar[1] =  get_value(argv[optind + 1]);
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

      ab.ipar[0] = long_flag; 

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
      ab.ipar[0] = get_value(argv[optind + 1]);

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
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_rolloverlimit") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_ROLLOVERLIMIT;
      ab.ipar[0] = get_value(argv[optind + 1]);

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
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_define_buffersize") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_DEFINE_BUFFERSIZE;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }


  else if ( strcasecmp(command,"daq_set_adaptivebuffering") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SETADAPTIVEBUFFER;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_webcontrol") == 0)
    {

      ab.action = DAQ_WEBCONTROL;
      if ( argc == optind + 2)
	{
	  ab.ipar[0] = get_value(argv[optind + 1]);
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

  else if ( strcasecmp(command,"daq_get_lastfilename") == 0)
    {

      ab.action = DAQ_GETLASTFILENAME;

      ab.ipar[0] = 0;

      r = r_action_1 (&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_get_last_event_number") == 0)
    {

      ab.action = DAQ_GETLASTEVENTNUMBER;

      ab.ipar[0] = 0;

      r = r_action_1 (&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_eventformat") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SETEVENTFORMAT;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_set_runcontrolmode") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_RUNCONTROLMODE;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_get_runcontrolmode") == 0)
    {

      ab.action = DAQ_GET_RUNCONTROLMODE;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

#ifdef HAVE_MOSQUITTO_H
  else if ( strcasecmp(command, "daq_set_mqtt_host") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_SET_MQTT_HOST;
      ab.spar = argv[optind + 1];
      if ( argc == optind + 3)
        {
          ab.ipar[0] = get_value(argv[optind + 2]);
        }
      else
        {
          ab.ipar[0] = 1883;
        }
      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL)
        {
          clnt_perror (clnt, "call failed");
        }
      if (r->content) std::cout <<  r->str << std::flush;

    }

  else if ( strcasecmp(command, "daq_get_mqtt_host") == 0)
    {
      ab.action = DAQ_GET_MQTT_HOST;

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL)
        {
          clnt_perror (clnt, "call failed");
        }
      if (r->content) std::cout <<  r->str << std::flush;

    }
#endif

  
  else if ( strcasecmp(command,"elog") == 0)
    {

      if ( argc != optind + 4) return -1;

      ab.action = DAQ_ELOG;
      ab.spar = argv[optind + 1];
      ab.ipar[0] = get_value(argv[optind + 2]);
      ab.spar2 = argv[optind +3];

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }
  

  else if ( strcasecmp(command,"load") == 0)
    {

      if ( argc != optind + 2) return -1;

      ab.action = DAQ_LOAD;
      ab.spar = argv[optind + 1];

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

  else if ( strcasecmp(command,"daq_set_uservalue") == 0)
    {
      if ( argc < optind + 3) return -1;

      ab.action = DAQ_SET_USERVALUE;
      ab.ipar[0] = get_value(argv[optind + 1]);
      ab.ipar[1] = get_value(argv[optind + 2]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
    }

  else if ( strcasecmp(command,"daq_get_uservalue") == 0)
    {
      if ( argc < optind + 2) return -1;

      ab.action = DAQ_GET_USERVALUE;
      ab.ipar[0] = get_value(argv[optind + 1]);

      r = r_action_1(&ab, clnt);
      if (r == (shortResult *) NULL) 
	{
	  clnt_perror (clnt, "call failed");
	}
      if (r->content) std::cout <<  r->str << std::flush;
 
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
