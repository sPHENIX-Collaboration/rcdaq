
#include <iostream>
#include "rcdaq.h"
#include <stdlib.h>

#include "daq_device.h" 
#include "daq_device_random.h" 

using namespace std;


void exitmsg()
{
  cout << "Usage: rcdaq [options] filerule" << endl;
  cout << "       rcdaq -h for help" << endl;
  exit(0);
}

void exithelp()
{
  cout << "** This is the DAQ engine. No gimmicks. Pure Power." << endl;
  cout << "** usage: rcdaq [options] filerule" << endl;
  cout << "   -m <minutes> run for so many minutes" << endl;
  cout << "   -b <number> read so many buffers" << endl;
  cout << "   -r <number> run number" << endl;
  cout << "   -p <id>   packet id to use (def 1001)" << endl;

  cout << "   -s file size chunks in MB (def 2GB)" << endl;
  cout << "   -v verbose" << endl;
  exit(0);
}



int main( int argc, char* argv[])
{

  int status;

  //  if (argc < 2) exitmsg();

  extern char *optarg;
  extern int optind;

  int i;

  /*
  char c;

  while ((c = getopt(argc, argv, "m:s:r:p:b:vh")) != EOF)
    switch (c) 
      {

      case 'm':
	{  
	  if ( !sscanf(optarg, "%d", &max_seconds) ) exitmsg();
	  max_seconds *= 60; // seconds
	}
	break;

      case 'b':
	{  
	  if ( !sscanf(optarg, "%d", &max_buffers) ) exitmsg();
	}
	break;

      case 'r':
	{  
	  if ( !sscanf(optarg, "%d", &runnumber) ) exitmsg();
	}
	break;

      case 'p':
	{  
	  if ( !sscanf(optarg, "%d", &packetid) ) exitmsg();
	}
	break;

      case 'v':   // verbose
	verbose++;
	break;

      case 'h':
	exithelp();
	break;
      }


  if (verbose) cout << "Opening file " << argv[optind] << endl;

  */


  rcdaq_init();


  add_readoutdevice ( new daq_device_random( 1,1001));
  daq_list_readlist();

  add_readoutdevice ( new daq_device_random( 1,1010,32, 0, 1024));
  add_readoutdevice ( new daq_device_random( 1,1011,64, 0, 4096));

  daq_list_readlist();

  daq_clear_readlist();
  add_readoutdevice ( new daq_device_random( 1,2010,32, 0, 1024));
  add_readoutdevice ( new daq_device_random( 1,2011,64, 0, 4096));

  daq_list_readlist();

  daq_open_file("xx.evt");
  daq_begin(100);
  daq_fake_trigger (10);

  daq_end();

  sleep (10);

  daq_close_file();

   
  return 0;
 
}
