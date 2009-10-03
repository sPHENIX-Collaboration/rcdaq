
#include "rcdaq_rpc.h"
#include "rcdaq_actions.h"

#include "rcdaq.h"
#include "daq_device.h" 
#include "all.h" 


#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <stdlib.h>

#include <rpc/pmap_clnt.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pthread.h"
#include "signal.h"


#include <set>
#include <iostream>
#include <fvtxserver.h>

using namespace std;

static pthread_mutex_t M_output;


void rcdaq_1(struct svc_req *rqstp, register SVCXPRT *transp);


//-------------------------------------------------------------

int server_setup(int argc, char **argv)
{
  return 0;
}

//-------------------------------------------------------------


shortResult * r_create_device_1_svc(actionblock *ab, struct svc_req *rqstp)
{
  static shortResult  result;

  result.str=" ";
  result.content = 0;
  result.what = 0;
  result.status = 0;


  switch (ab->spare)
    {
    case DAQ_DEVICE_RANDOM:
      switch ( ab->ipar[15])
	{

	case 2:
	  add_readoutdevice ( new daq_device_random( ab->ipar[0],
						     ab->ipar[1]));
	  break;

	case 3:
	  add_readoutdevice ( new daq_device_random( ab->ipar[0],
						     ab->ipar[1],
						     ab->ipar[2]));
	  break;

	case 4:
	  add_readoutdevice ( new daq_device_random( ab->ipar[0],
						     ab->ipar[1],
						     ab->ipar[2],
						     ab->ipar[3]));
	  break;

	case 5:
	  add_readoutdevice ( new daq_device_random( ab->ipar[0],
						     ab->ipar[1],
						     ab->ipar[2],
						     ab->ipar[3],
						     ab->ipar[4]));
	  break;

	default:
	  result.content=1;
	  result.str= "Wrong parameters for daq_device_random";
	  return &result;
	  break;
	}
      break;

    case DAQ_DEVICE_FILE:
      add_readoutdevice ( new daq_device_file( ab->ipar[0],
					       ab->ipar[1],
					       ab->spar));
      break;

    default:
      result.content=1;
      result.str= "Unknown device";
      return &result;
      break;

    }
  return &result;
}

//-------------------------------------------------------------

shortResult * r_action_1_svc(actionblock *ab, struct svc_req *rqstp)
{

  static shortResult  result;

  static std::ostringstream outputstream;


  std::cout << ab->action << std::endl;

  pthread_mutex_lock(&M_output);
  result.str=" ";
  result.content = 0;
  result.what = 0;
  result.status = 0;

  int status;

  outputstream.str("");

  switch ( ab->action)
    {
      
    case DAQ_BEGIN:
      //  cout << "daq_begin " << ab->ipar[0] << endl;
      result.status = daq_begin (  ab->ipar[0], outputstream);
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_END:
        // cout << "daq_end "  << endl;
      result.status = daq_end(outputstream);
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETFILERULE:
      // cout << "daq_set_filerule " << ab->spar << endl;
      daq_set_filerule(ab->spar);
      break;

    case DAQ_OPEN:
      // cout << "daq_open " << ab->spar << endl;
      result.status = daq_open(outputstream);
      if (result.status) 
	{
	  result.str = (char *) outputstream.str().c_str();
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_CLOSE:
      // cout << "daq_close "  << endl;
      result.status = daq_close(outputstream);
      if (result.status) 
	{
	  result.str = (char *) outputstream.str().c_str();
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_FAKETRIGGER:
      // cout << "daq_fake_trigger "  << endl;
      result.status =  daq_fake_trigger (ab->ipar[0], ab->ipar[1]);
      break;

    case DAQ_LISTREADLIST:
      // cout << "daq_list_readlist "  << endl;
      result.status = daq_list_readlist(outputstream);
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_CLEARREADLIST:
      // cout << "daq_clear_readlist "  << endl;
      result.status = daq_clear_readlist(outputstream);
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_STATUS:

      cout << "daq_status "  << endl;
      result.status = daq_status(ab->ipar[0], outputstream);
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXEVENTS:
      //  cout << "daq_setmaxevents " << ab->ipar[0] << endl;
      result.status = daq_setmaxevents (  ab->ipar[0], outputstream);
      result.str = (char *) outputstream.str().c_str();
      if (result.status) 
	{
	  result.str = (char *) outputstream.str().c_str();
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXVOLUME:
      //  cout << "daq_setmaxvolume " << ab->ipar[0] << endl;
      result.status = daq_setmaxvolume (  ab->ipar[0], outputstream);
      result.str = (char *) outputstream.str().c_str();
      if (result.status) 
	{
	  result.str = (char *) outputstream.str().c_str();
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;


    case DAQ_ELOG:
      // cout << "daq_elog " << ab->spar << "  " << ab->ipar[0] << endl;
      daq_set_eloghandler( ab->spar,  ab->ipar[0], ab->spar2);
      break;


    default:
      result.str =   "Unknown action";
      result.content = 1;
      result.status = 1;
      break;

    }
  
  pthread_mutex_unlock(&M_output);
  return &result;
  

}

//-------------------------------------------------------------


int
main (int argc, char **argv)
{

  int i;

  pthread_mutex_init(&M_output, 0); 

  rcdaq_init();


  server_setup(argc, argv);

  int servernumber = 0;

  if ( argc > 1)
    {
      servernumber = atoi(argv[1]);
    }
  std::cout << "Server number is " << servernumber << std::endl;

  register SVCXPRT *transp;
  
  pmap_unset (RCDAQ+servernumber, RCDAQ_VERS);
    
  
  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf (stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, RCDAQ+servernumber, RCDAQ_VERS, rcdaq_1, IPPROTO_TCP)) {
    fprintf (stderr, "%s", "unable to register (RCDAQ+servernumber, RCDAQ_VERS, tcp).");
    exit(1);
  }
  
  svc_run ();
  fprintf (stderr, "%s", "svc_run returned");
  exit (1);
  /* NOTREACHED */
}

