
#include "rcdaq_rpc.h"
#include "rcdaq_actions.h"

#include "parseargument.h"
#include "rcdaq.h"
#include "daq_device.h" 
#include "rcdaq_plugin.h" 
#include "all.h" 


#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <rpc/pmap_clnt.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pthread.h"
#include "signal.h"

#include <vector>


#include <set>
#include <iostream>

using namespace std;

static pthread_mutex_t M_output;

std::vector<RCDAQPlugin *> pluginlist;

static unsigned long  my_servernumber = 0;

void rcdaq_1(struct svc_req *rqstp, register SVCXPRT *transp);


//-------------------------------------------------------------

int server_setup(int argc, char **argv)
{
  return 0;
}

//-------------------------------------------------------------

void plugin_register(RCDAQPlugin * p )
{
  pluginlist.push_back(p);
}

void plugin_unregister(RCDAQPlugin * p )
{
  // do nothing for now
  // we need to implement this once we add unloading of plugins
  // as a feature (don't see why at this point)
}

int daq_load_plugin( const char *sharedlib, std::ostream& os)
{
  void * v = dlopen(sharedlib, RTLD_GLOBAL | RTLD_NOW | RTLD_NOLOAD);
  if (v) 
    {
      std::cout << "Plugin " 
		<< sharedlib << " already loaded" << std::endl;
      os << "Plugin " 
		<< sharedlib << " already loaded" << std::endl;
      return 0;
    }
    
  void * voidpointer = dlopen(sharedlib, RTLD_GLOBAL | RTLD_NOW);
  if (!voidpointer) 
    {
      std::cout << "Loading of the plugin " 
		<< sharedlib << " failed: " << dlerror() << std::endl;
      os << "Loading of the plugin " 
		<< sharedlib << " failed: " << dlerror() << std::endl;
      return -1;
      
    }
  os << "Plugin " << sharedlib << " successfully loaded" << std::endl;
  cout  << "Plugin " << sharedlib << " successfully loaded" << std::endl;
  return 0;

}

//-------------------------------------------------------------

// for certain flags we add some info to the status output
// with this routine. It is called from daq_status. 

int daq_status_plugin (const int flag, std::ostream& os )
{

  // in case we do have plugins, list them
  // if not, we just say "no plugins loded"
  if (   pluginlist.size() )
    {
      os << "List of loaded Plugins:" << endl;
    }
  else
    {
      os << "No Plugins loaded" << endl;
    }


  std::vector<RCDAQPlugin *>::iterator it;

  for ( it=pluginlist.begin(); it != pluginlist.end(); ++it)
    {
      (*it)->identify(os, flag);
    }
  return 0;
}

//-------------------------------------------------------------


shortResult * r_create_device_1_svc(deviceblock *db, struct svc_req *rqstp)
{
  static shortResult  result, error;
  static char e_string[512];
  static char r_string[512];
  error.str = e_string;
  result.str = r_string;
  strcpy ( r_string, " ");
  
  if ( daq_running() )
    {
      strcpy(e_string, "Run is active");
      error.content = 1;
      error.what    = 0;
      error.status  = -1;
      return &error;
    }

  strcpy(e_string, "Device needs at least 2 parameters");
  error.content = 1;
  error.what    = 0;
  error.status  = -1;

  result.status = 0;
  result.what   = 0;
  result.content= 0;


  int eventtype;
  int subid;

  int ipar[16];
  int i;


  if ( db->npar < 3)
    {
      strcpy(r_string, "Device needs at least 2 parameters");
      return &error;
    }
  
      
  strcpy(r_string, "Wrong number of parameters");

  // and we decode the event type and subid
  eventtype  = get_value ( db->argv1); // event type
  subid = get_value ( db->argv2); // subevent id


  // now we will see what device we are supposed to set up.
  // we first check if it is one of our built-in ones, such as
  // device_random, or device_file or so.
  

  if ( strcasecmp(db->argv0,"device_random") == 0 ) 
    {

      // this happens to be the most complex contructor part
      // so far since there are a few variants, 2-6 parameters
      switch ( db->npar)
	{
	case 3:
          add_readoutdevice ( new daq_device_random( eventtype,
                                                     subid ));
          break;

        case 4:
          add_readoutdevice ( new daq_device_random( eventtype,
                                                     subid,
						     get_value ( db->argv3)));
          break;

        case 5:
          add_readoutdevice ( new daq_device_random( eventtype,
                                                     subid, 
						     get_value ( db->argv3),
						     get_value ( db->argv4)));
          break;

        case 6:
          add_readoutdevice ( new daq_device_random( eventtype,
                                                     subid, 
						     get_value ( db->argv3),
						     get_value ( db->argv4),
						     get_value ( db->argv5)));
          break;

        case 7:
          add_readoutdevice ( new daq_device_random( eventtype,
                                                     subid, 
						     get_value ( db->argv3),
						     get_value ( db->argv4),
						     get_value ( db->argv5),
						     get_value ( db->argv6)));
          break;

       default:
          return &error;
          break;
        }

      return &result;
    }

  if ( strcasecmp(db->argv0,"device_deadtime") == 0 ) 
    {

      // this happens to be the most complex contructor part
      // so far since there are a few variants, 2-6 parameters
      switch ( db->npar)
	{
	case 3:
          add_readoutdevice ( new daq_device_deadtime( eventtype,
						       subid ));
          break;

        case 4:  // plus number of ticks
          add_readoutdevice ( new daq_device_deadtime( eventtype,
						       subid,
						       get_value ( db->argv3)));
          break;

        case 5:  // plus number of words
          add_readoutdevice ( new daq_device_deadtime( eventtype,
						       subid, 
						       get_value ( db->argv3),
						       get_value ( db->argv4)));
          break;

        case 6:   // plus trigger flag
          add_readoutdevice ( new daq_device_deadtime( eventtype,
						       subid, 
						       get_value ( db->argv3),
						       get_value ( db->argv4),
						       get_value ( db->argv5)));
          break;

       default:
          return &error;
          break;
        }

      return &result;
    }


  else if ( strcasecmp(db->argv0,"device_file") == 0 )  
    {

      if ( db->npar < 4) return &error;

      if ( db->npar >= 5) 
	{
	  // we give the size in kbytes but we want it in words 
	  int s = 1024*(get_value(db->argv4)+3)/4;
	  if ( s < 4*1024) s = 4*1024;    // this is the default size
	  add_readoutdevice ( new daq_device_file( eventtype,
						   subid,
						   db->argv3,
						   0, // no delete
						   s ) );
	  return &result;
	}
      else 
	{

	  add_readoutdevice ( new daq_device_file( eventtype,
						   subid,
						   db->argv3));
	  return &result;
	}

    }
  // --------------------------------------------------------------------------
  // although this logic is very similar to device_file, since the consequences
  // of a misplaced parameter are so severe, we make a new device
  else if ( strcasecmp(db->argv0,"device_file_delete") == 0 )  
    {

      if ( db->npar < 4) return &error;

      if ( db->npar >= 5) 
	{
	  // we give the size in kbytes but we want it in words 
	  int s = 1024*(get_value(db->argv4)+3)/4;
	  if ( s < 4*1024) s = 4*1024;    // this is the default size
	  add_readoutdevice ( new daq_device_file( eventtype,
						   subid,
						   db->argv3,
						   1,  // we add the delete flag
						   s ) );
	  return &result;
	}
      else 
	{

	  add_readoutdevice ( new daq_device_file( eventtype,
						   subid,
						   db->argv3,
						   1) );
	  return &result;
	}

    }


  // --------------------------------------------------------------------------


  else if ( strcasecmp(db->argv0,"device_filenumbers") == 0 )  
    {

      if ( db->npar < 4) return &error;

      if ( db->npar >= 5) 
	{
	  int s = 1024*(get_value(db->argv4)+3)/4;
	  if ( s < 256) s = 256;    // this is the default size

	  add_readoutdevice ( new daq_device_filenumbers( eventtype,
							  subid,
							  db->argv3,
							  0,  // no delete
							  s));
	  return &result;
	}
      else 
	{

	  add_readoutdevice ( new daq_device_filenumbers( eventtype,
							  subid,
							  db->argv3));
	  return &result;
	}

    }

  // --------------------------------------------------------------------------


  else if ( strcasecmp(db->argv0,"device_filenumbers_delete") == 0 )  
    {

      if ( db->npar < 4) return &error;

      if ( db->npar >= 5) 
	{
	  int s = 1024*(get_value(db->argv4)+3)/4;
	  if ( s < 256) s = 256;    // this is the default size

	  add_readoutdevice ( new daq_device_filenumbers( eventtype,
							  subid,
							  db->argv3,
							  1,  // we add the delete flag
							  s));
	  return &result;
	}
      else 
	{

	  add_readoutdevice ( new daq_device_filenumbers( eventtype,
							  subid,
							  db->argv3,
							  1) );
	  return &result;
	}

    }


  else if ( strcasecmp(db->argv0,"device_command") == 0 )  
    {

      if ( db->npar < 4) return &error;

      if ( db->npar >= 5) 
	{
	  int s = (get_value(db->argv4)+3)/4;
	  if ( s < 1280) s = 1280;    // this is the default size
	  add_readoutdevice ( new daq_device_file( eventtype,
						   subid,
						   db->argv3,
						   get_value(db->argv4) ) );
	  return &result;
	}
      else 
	{

	  add_readoutdevice ( new daq_device_command( eventtype,
						   subid,
						   db->argv3));
	  return &result;
	}

    }
  else if ( strcasecmp(db->argv0,"device_rtclock") == 0 )  
    {
      
      add_readoutdevice ( new daq_device_rtclock( eventtype,
					       subid));
			  
      return &result;
    }


  // nada, it was none of the built-in ones if we arrive here.

  // we now go through through the list of plugins and see if any one of
  // our plugins can make the device.

  // there are three possibilities:
  //  1) the plugin can make the device, all done. In that case, return = 0 
  //  2) the plugin can make the device but the parameters are wrong, return  = 1 
  //  3) the plugin dosn not knwo about tha device, we keep on going...  return = 2

  // we keep doing that until we either find a plugin that knows the device or  we run
  // out of plugins
  
  std::vector<RCDAQPlugin *>::iterator it;

  int status;
  for ( it=pluginlist.begin(); it != pluginlist.end(); ++it)
    {
      status = (*it)->create_device(db);
      // in both of the following cases we are done here:
      if (status == 0) return &result;  // sucessfully created 
      else if ( status == 1) return &error;  // it's my device but wrong params
      // in all other cases we continue and try the next plugin
    }

  result.content=1;
  strcpy(r_string, "Unknown device");
  return &result;

}

//-------------------------------------------------------------

shortResult * r_action_1_svc(actionblock *ab, struct svc_req *rqstp)
{

  static shortResult  result;

  static std::ostringstream outputstream;
  static int currentmaxresultlength = 10*2048;
  static char *resultstring = new char[currentmaxresultlength+1];

  
  if ( outputstream.str().size() > currentmaxresultlength )
    {
      currentmaxresultlength = outputstream.str().size();
      std::cout << __LINE__<< "  " << __FILE__ 
		<< " *** warning: extended result string to " << currentmaxresultlength << std::endl;
      delete [] resultstring;
      resultstring = new char[currentmaxresultlength+1];
    }


  pthread_mutex_lock(&M_output);
  result.str=resultstring;
  strcpy(resultstring," ");
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
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_END:
        // cout << "daq_end "  << endl;
      result.status = daq_end(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_RUNNUMBERFILE:
      daq_set_runnumberfile(ab->spar);
      break;

    case DAQ_SETFILERULE:
      daq_set_filerule(ab->spar);
      break;

    case DAQ_SETNAME:
      daq_set_name(ab->spar);
      break;


    case DAQ_SETRUNTYPE:
      result.status = daq_setruntype(ab->spar,outputstream);
      if ( result.status)
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result; 
      break;

    case DAQ_GETRUNTYPE:
      result.status = daq_getruntype(ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_DEFINERUNTYPE:
      daq_define_runtype(ab->spar, ab->spar2);
      break;

    case DAQ_LISTRUNTYPES:
      // cout << "daq_list_runtypes "  << endl;
      result.status = daq_list_runtypes(ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_GETNAME:
      result.status = daq_get_name(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

      
    case DAQ_OPEN:
      // cout << "daq_open " << ab->spar << endl;
      result.status = daq_open(outputstream);
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
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
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
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
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_CLEARREADLIST:
      // cout << "daq_clear_readlist "  << endl;
      result.status = daq_clear_readlist(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_STATUS:

      //      cout << "daq_status "  << endl;
      result.status = daq_status(ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXEVENTS:
      //  cout << "daq_setmaxevents " << ab->ipar[0] << endl;
      result.status = daq_setmaxevents (  ab->ipar[0], outputstream);
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXVOLUME:
      //  cout << "daq_setmaxvolume " << ab->ipar[0] << endl;
      result.status = daq_setmaxvolume (  ab->ipar[0], outputstream);
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXBUFFERSIZE:
      //  cout << "daq_setmaxvolume " << ab->ipar[0] << endl;
      result.status = daq_setmaxbuffersize (  ab->ipar[0], outputstream);
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETADAPTIVEBUFFER:
      //  cout << "daq_setmaxevents " << ab->ipar[0] << endl;
      result.status = daq_setadaptivebuffering (  ab->ipar[0], outputstream);
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;


    case DAQ_ELOG:
      // cout << "daq_elog " << ab->spar << "  " << ab->ipar[0] << endl;
      daq_set_eloghandler( ab->spar,  ab->ipar[0], ab->spar2);
      break;

    case DAQ_LOAD:
      // 
      result.status = daq_load_plugin( ab->spar, outputstream );
      if (result.status) 
	{
	  outputstream.str().copy(resultstring,outputstream.str().size());
	  resultstring[outputstream.str().size()] = 0;
	  result.str = resultstring;
	  result.content = 1;
	}
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_WEBCONTROL:
      //  cout << "daq_begin " << ab->ipar[0] << endl;
      result.status = daq_webcontrol (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_GETLASTFILENAME:
      result.status = daq_getlastfilename ( outputstream);
      if ( result.status )
	{
	  pthread_mutex_unlock(&M_output);
	  result.content = 0;
	  return &result;
	}
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    default:
      strcpy(resultstring, "Unknown action");
      result.content = 1;
      result.status = 1;
      break;

    }
  
  pthread_mutex_unlock(&M_output);
  return &result;
  

}

shortResult * r_shutdown_1_svc(void *x, struct svc_req *rqstp)
{

  
  static shortResult  result;
  
  static std::ostringstream outputstream;
  

  pthread_mutex_lock(&M_output);

  
  result.content = 0;
  result.what = 0;
  result.status = 0;

  int status;

  outputstream.str("");
  result.str = (char *) outputstream.str().c_str();

  result.status = daq_shutdown ( my_servernumber, RCDAQ_VERS, outputstream);
  cout << "daq_shutdown status = " << result.status  << endl;
  if (result.status) 
    {
      result.str = (char *) outputstream.str().c_str();
      result.content = 1;
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

  rcdaq_init( M_output);


  server_setup(argc, argv);

  int servernumber = 0;

  if ( argc > 1)
    {
      servernumber = get_value(argv[1]);
    }
  std::cout << "Server number is " << servernumber << std::endl;

  my_servernumber = RCDAQ+servernumber;  // remember who we are for later

  register SVCXPRT *transp;
  
  pmap_unset (my_servernumber, RCDAQ_VERS);

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf (stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, my_servernumber, RCDAQ_VERS, rcdaq_1, IPPROTO_TCP)) {
    fprintf (stderr, "%s", "unable to register (RCDAQ+servernumber, RCDAQ_VERS, tcp).");
    exit(1);
  }

  char hostname[1024];
  i = gethostname(hostname, 1024);
  i = daq_set_name(hostname);

  svc_run ();
  fprintf (stderr, "%s", "svc_run returned");
  exit (1);
  /* NOTREACHED */
}

