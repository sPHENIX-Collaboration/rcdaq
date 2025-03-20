
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

#include <sys/file.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>


#include <vector>


#include <set>
#include <iostream>

using namespace std;

static pthread_mutex_t M_output;

std::vector<RCDAQPlugin *> pluginlist;

static unsigned long  my_servernumber = 0;

extern "C"  void rcdaq_1(struct svc_req *rqstp, SVCXPRT *transp);

static int pid_fd = 0;


void sig_handler(int i)
{
  if (pid_fd)
    {
      char *pidfilename = obtain_pidfilename();
      unlink (pidfilename);
    }
  exit(0);
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
      os << "  List of loaded Plugins:" << endl;
    }
  else
    {
      os << "   No Plugins loaded" << endl;
    }


  std::vector<RCDAQPlugin *>::iterator it;

  for ( it=pluginlist.begin(); it != pluginlist.end(); ++it)
    {
      os << "  ";
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
      strcpy(e_string, "Run is active\n");
      error.content = 1;
      error.what    = 0;
      error.status  = -1;
      return &error;
    }

  strcpy(e_string, "Device needs at least 2 parameters\n");
  error.content = 1;
  error.what    = 0;
  error.status  = -1;

  result.status = 0;
  result.what   = 0;
  result.content= 0;


  int eventtype;
  int subid;


  if ( db->npar < 3)
    {
      strcpy(r_string, "Device needs at least 2 parameters\n");
      return &error;
    }
  
      
  strcpy(r_string, "Wrong number of parameters\n");

  // and we decode the event type and subid
  eventtype  = get_value ( db->argv1); // event type
  subid = get_value ( db->argv2); // subevent id


  // now we will see what device we are supposed to setup.
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
  static unsigned int currentmaxresultlength = 10*2048;
  static char *resultstring = new char[currentmaxresultlength+1];

  // to avoid a race condition with the asynchronous
  // begin or "end requested" feature, wait here...
  daq_wait_for_begin_done();
  daq_wait_for_actual_end();

  
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

  outputstream.str("");

  switch ( ab->action)
    {
      
    case DAQ_SYNC:
      pthread_mutex_unlock(&M_output);

      return &result;
      break;

    case DAQ_BEGIN:
      result.status = daq_begin (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_BEGIN_IMMEDIATE:
      result.status = daq_begin_immediate (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_END:
      result.status = daq_end_interactive(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_END_IMMEDIATE:
      result.status = daq_end_immediate(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_RUNNUMBERFILE:
      daq_set_runnumberfile(ab->spar, ab->ipar[0]);
      break;

    case DAQ_SETRUNNUMBERAPP:
      daq_set_runnumberApp(ab->spar, ab->ipar[0]);
      break;

    case DAQ_SETFILERULE:
      result.status = daq_set_filerule(ab->spar, outputstream);
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

    case DAQ_SETNAME:
      daq_set_name(ab->spar);
      break;

#ifdef HAVE_MOSQUITTO_H
      
    case DAQ_SET_MQTT_HOST:
      result.status = daq_set_mqtt_host(ab->spar, ab->ipar[0], outputstream);
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


    case DAQ_GET_MQTT_HOST:
      result.status = daq_get_mqtt_host(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;
#endif
      
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

    case DAQ_SET_SERVER:
      result.status = daq_set_server(ab->spar, ab->ipar[0], outputstream);
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

    case DAQ_SET_COMPRESSION:
      result.status = daq_set_compression (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SET_NR_THREADS:
      result.status = daq_set_nr_threads (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SET_MD5ENABLE:
      result.status = daq_set_md5enable (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SET_MD5ALLOWTURNOFF:
      result.status = daq_set_md5allowturnoff (  ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;
      
    case DAQ_FAKETRIGGER:
      result.status =  daq_fake_trigger (ab->ipar[0], ab->ipar[1]);
      break;

    case DAQ_LISTREADLIST:
      result.status = daq_list_readlist(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_CLEARREADLIST:
      result.status = daq_clear_readlist(outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_STATUS:
      result.status = daq_status(ab->ipar[0], outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETMAXEVENTS:
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

    case DAQ_ROLLOVERLIMIT:
      result.status = daq_setrolloverlimit (  ab->ipar[0], outputstream);
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

    case DAQ_DEFINE_BUFFERSIZE:
      result.status = daq_define_buffersize (  ab->ipar[0], outputstream);
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
      daq_set_eloghandler( ab->spar,  ab->ipar[0], ab->spar2);
      break;

    case DAQ_LOAD:
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

    case DAQ_GETLASTEVENTNUMBER:
      result.what = daq_getlastevent_number ( outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      result.status = 0;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SETEVENTFORMAT:
      result.status = daq_setEventFormat ( ab->ipar[0], outputstream);
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

    case DAQ_SET_RUNCONTROLMODE:
      result.status = daq_setRunControlMode ( ab->ipar[0], outputstream);
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

    case DAQ_GET_RUNCONTROLMODE:
      result.status = daq_getRunControlMode (outputstream);
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
      pthread_mutex_unlock(&M_output);
      return &result;
      break;

    case DAQ_SET_USERVALUE:
      result.status = daq_set_uservalue (  ab->ipar[0], ab->ipar[1],  outputstream);
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

    case DAQ_GET_USERVALUE:
      result.status = daq_get_uservalue (  ab->ipar[0],  outputstream);
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
  
  static char resultstring[256];

  pthread_mutex_lock(&M_output);

  
  result.content = 0;
  result.what = 0;
  result.status = 0;

  outputstream.str("");

  result.str = (char *) outputstream.str().c_str();

  result.status = daq_shutdown ( my_servernumber, RCDAQ_VERS, pid_fd, outputstream);
  cout << "daq_shutdown status = " << result.status  << endl;
  if (result.status) 
    {
      outputstream.str().copy(resultstring,outputstream.str().size());
      resultstring[outputstream.str().size()] = 0;
      result.str = resultstring;
      result.content = 1;
    }
  pthread_mutex_unlock(&M_output);
  return &result;

}

//-------------------------------------------------------------


int
main (int argc, char **argv)
{

  int servernumber = 0;

  mode_t mask = umask(0);
  int i = mkdir ( "/tmp/rcdaq", 0777);
  if ( i && errno != EEXIST)
    {
      std::cerr << "Error accessing the lock directory /tmp/rcdaq" << std::endl;
      return 2;
    }

  umask(mask);
  
  if ( argc > 1)
    {
      servernumber = get_value(argv[1]);
    }

  char *pidfilename = obtain_pidfilename();
  
  sprintf (pidfilename, "/tmp/rcdaq/rcdaq_%d", servernumber);

  pid_fd = open(pidfilename, O_CREAT | O_RDWR, 0666);
  if ( pid_fd < 0)
    {

      ifstream pidfile = ifstream(pidfilename);
      if ( pidfile.is_open())
	{
	  int ipid;
	  pidfile >> ipid;
	  std::cerr << "Another server is already running, PID= " << ipid << std::endl;
	  return 2;
	}
  
      std::cerr << "Error creating the lock file" << std::endl;
      return 2;
    }
  
  int rc = flock(pid_fd, LOCK_EX | LOCK_NB);
  if(rc)
    {
      if (errno == EWOULDBLOCK)
	{
	  ifstream pidfile = ifstream(pidfilename);
	  if ( pidfile.is_open())
	    {
	      int ipid;
	      pidfile >> ipid;
	      std::cerr << "Another server is already running, PID= " << ipid << std::endl;
	      return 3;
	    }
	  
	  std::cerr << "Another server is already running" << std::endl;
	  return 3;
	}
    }

  // we write out pid in here, for good measure
  char pid[64];
  int x = sprintf(pid,"%d\n", getpid());
  write (pid_fd, pid, x);
  
  
  std::cout << "Server number is " << servernumber << std::endl;

  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT,  sig_handler);
  
  
  pthread_mutex_init(&M_output, 0); 

  rcdaq_init( servernumber, M_output);

  my_servernumber = RCDAQ+servernumber;  // remember who we are for later

  SVCXPRT *transp;
  
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

  // char hostname[1024];
  // gethostname(hostname, 1024);
  // daq_set_name(hostname);

  svc_run ();
  fprintf (stderr, "%s", "svc_run returned");
  exit (1);
  /* NOTREACHED */
}

