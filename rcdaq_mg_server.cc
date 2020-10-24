
#include "mongoose.h"
#include "rcdaq.h"

#include <iostream>
#include <sstream>
#include <sys/stat.h>

using namespace std;

int update(struct mg_connection *nc, const char *key, const double value);
int update(struct mg_connection *nc, const char *key, const int value);
int update(struct mg_connection *nc, const char *key, const char *value);
int trigger_updates(struct mg_connection *nc);

void initial_ws_update (struct mg_connection *nc);
void send_ws_updates (struct mg_connection *nc);
void send_updates (struct mg_connection *nc);
void send_status (struct mg_connection *nc, std::string out);
void send_error (struct mg_connection *nc, std::string out);
void send_nothing (struct mg_connection *nc);
void * mg_server (void *arg);

static void broadcast(struct mg_connection *nc, char *str, const int len);


static struct mg_serve_http_opts s_http_server_opts;

int end_web_thread = 0;

pthread_mutex_t M_ws_send;

int error_flag = 0;
std::string error_string = "";
int initial_update_flag = 0;

int mg_end()
{
  end_web_thread = 1;
  return 0;
}


static int is_websocket(const struct mg_connection *nc)
{
  return nc->flags & MG_F_IS_WEBSOCKET;
}

std::string get_statusstring()
{
  stringstream out;

  if ( error_flag)
    {
      return  error_string;
    }
  else if ( daq_running() )
    {
      out << "Running for " << get_runduration() << "s" << ends;
    }
  else
    {
      out << "Stopped Run " << get_oldrunnumber() << ends;
    }
  return out.str();
}

std::string get_loggingstring()
{
  stringstream out;
  if ( get_openflag() )
    {
      if ( daq_running() )
	{
	  out << "File: " << get_current_filename();
	  return out.str();
	}
      else
	{
	  return "Logging enabled";
	}
    }

  return "Logging disabled";
}


void initial_ws_update (struct mg_connection *nc)
{
  char str[2048];
  int len;
  len = sprintf(str, "{ \"RunFlag\":%d, \"Status\":\"%s\" , \"RunNr\":%d , \"Events\":%d , \"Volume\":\"%f\", \"Duration\":%d, \"Logging\":\"%s\" ,\"Filename\":\"%s \" , \"OpenFlag\":%d , \"Name\":\"%s\"  } "
		, daq_running()
		, get_statusstring().c_str()
		, get_runnumber()
		, get_eventnumber()
		, get_runvolume()
		, get_runduration()
		, get_loggingstring().c_str()
		, get_current_filename().c_str()
		, get_openflag()
		, daq_get_myname().c_str()
		);
  //  cout << __FILE__ << " " << __LINE__ << " " << str << endl;
  
  broadcast(nc, str, len);
  
}

static void broadcast(struct mg_connection *nc, char *str, const int len)
{
  struct mg_connection *c;

  for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c))
    {
      if ( c != nc)
	{
	  //	   cout << __FILE__ << " " << __LINE__ << " sending " << str << " to " << c << endl;

	  //	   pthread_mutex_lock(&M_ws_send); 
	  mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, str, len);
	  //pthread_mutex_unlock(&M_ws_send); 
	}
    }
}


int update(struct mg_connection *nc, const char *key, const double value)
{
  char str[512];
  int len = sprintf(str, "{ \"%s\":%f }", key, value);

  broadcast(nc, str, len);
  return 0;
}

int update(struct mg_connection *nc, const char *key, const int value)
{
  char str[512];
  int len = sprintf(str, "{ \"%s\":%d }", key, value);

  broadcast(nc, str, len);
  return 0;
}
  
int update(struct mg_connection *nc, const char *key, const char *value)
{
  char str[512];
  int len = sprintf(str, "{ \"%s\":\"%s \" }", key, value);

  broadcast(nc, str, len);
  return 0;
}
  
void send_ws_updates (struct mg_connection *nc)
{
  char str[512];
  int len;
  if ( daq_running() )
    {
      //      cout << __FILE__ << " " << __LINE__ << " in send_ws_updates" << endl;
      update(nc, "Volume", get_runvolume());
      update(nc, "Events", get_eventnumber());
      update(nc, "Status", get_statusstring().c_str() );
    }

}

void send_updates (struct mg_connection *nc)
{
  char str[512];
  int len;

  
  len = sprintf(str, "{ \"Events\":%d , \"Volume\":\"%f\" , \"Status\":\"%s\"  }"
		, get_eventnumber()
		, get_runvolume()
		, get_statusstring().c_str()
		);

  //cout << __FILE__ << " " << __LINE__ << " sending " << str << endl;
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
	    "Content-Type: application/json\r\n\r\n%s",
	    len, str);
  
}

void send_status (struct mg_connection *nc, std::string out)
{

  // we eliminate the line break
  ostringstream x;
  x<< endl;
  
  out.replace(out.find(x.str()),x.str().length(),"");
  //cout << __FILE__ << " " << __LINE__ << " " << out << endl;


  
  char str[512];
  int len;
  len = sprintf(str, "{ \"Status\":\"%s\" }"
		, out.c_str() );

  //  pthread_mutex_lock(&M_ws_send); 
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
	    "Content-Type: application/json\r\n\r\n%s",
	    len, str);
  // pthread_mutex_unlock(&M_ws_send); 
  
}

void send_error (struct mg_connection *nc, std::string out)
{

  // we eliminate the line break
  ostringstream x;
  x<< endl;
  
  out.replace(out.find(x.str()),x.str().length(),"");
  //cout << __FILE__ << " " << __LINE__ << " " << out << endl;


  
  //  pthread_mutex_lock(&M_ws_send); 
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "{ \"Status\":\"%s\" }", out.c_str());
  // pthread_mutex_unlock(&M_ws_send); 
  
}

void send_nothing (struct mg_connection *nc)
{
  //  pthread_mutex_lock(&M_ws_send); 
  //  printf(" sending HTTP/1.0 200 OK\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n");
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: 0\r\n"
	    "Content-Type: text/html\r\n\r\n");
  //pthread_mutex_unlock(&M_ws_send); 
  
}



//int has_request=0;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  struct http_message *hm = (struct http_message *) ev_data;
  struct websocket_message *wm = (struct websocket_message *)ev_data;
  char str[256];
  int i;
  int status;
   
  std::ostringstream out;
		 
  //   if (ev)  printf (" ----   %d\n" ,ev);
  //   printf (" --bit 20 --   %lx\n" , nc->flags & MG_F_USER_1);
  switch (ev)
    {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
      {
	/* New websocket connection. Tell everybody. */
	nc->flags |=  MG_F_USER_1;
	break;
      }

    case MG_EV_WEBSOCKET_FRAME:
      {
	struct mg_str msg = {(char *) wm->data, wm->size};
//	cout << __FILE__ << " " << __LINE__ << " ws message " << wm->data << endl;
	 
	if ( mg_vcmp ( &msg, "daq_begin") == 0)
	  {
	     
	    status = daq_begin(0,out);
	    if (status)
	      {
		error_flag = 1;
		error_string = out.str();
		error_string.replace(error_string.find("\n"),1,"");

	      }
	    else
	      {
		error_flag = 0;
		error_string = "";
	      }
	 
	    return;
	  }

	else if ( mg_vcmp ( &msg, "daq_end") == 0)
	  {
	    status = daq_end(out);
	    if (status)
	      {
		send_error(nc,out.str() );
	      }
	    return;
	  }

	else if ( mg_vcmp ( &msg, "daq_open") == 0)
	  {
	    //    cout << __FILE__ << " " << __LINE__ << " daq_open request" << endl;
	    daq_open();
	    return;
	  }
	 
	else if ( mg_vcmp ( &msg, "daq_close") == 0)
	  {
	    //    cout << __FILE__ << " " << __LINE__ << " daq_open request" << endl;
	    daq_close();
	    return;
	  }

	else if ( mg_vcmp ( &msg, "initial_update") == 0)
	  {
	    initial_update_flag= 1;
	    return;
	  }

	break;
      }

    case MG_EV_HTTP_REQUEST:
      {
	//cout << __FILE__ << " " << __LINE__ << " connection: " << nc << " uri: " << hm->uri.p << endl;
	 
	if ( mg_vcmp ( &hm->uri, "/send_updates") == 0)
	  {
	    send_updates(nc);
	    return;
	  }
	 
	else if ( mg_vcmp ( &hm->uri, "/daq_begin") == 0)
	  {
	    status = daq_begin(0,out);
	    if (status)
	      {
		send_status(nc,out.str() );
	      }
	    else
	      {
		send_nothing(nc);
	      }
	    return;
	  }

	else if ( mg_vcmp ( &hm->uri, "/daq_end") == 0)
	  {
	    status = daq_end(out);
	    if (status)
	      {
		send_status(nc,out.str() );
	      }
	    else
	      {
		send_nothing(nc);
		 
	      }
	    return;
	  }

	else if ( mg_vcmp ( &hm->uri, "/daq_open") == 0)
	  {
	    //    cout << __FILE__ << " " << __LINE__ << " daq_open request" << endl;
	    daq_open();
	    send_nothing(nc);
	    return;
	  }
	 
	else if ( mg_vcmp ( &hm->uri, "/daq_close") == 0)
	  {
	    //    cout << __FILE__ << " " << __LINE__ << " daq_open request" << endl;
	    daq_close();
	    send_nothing(nc);
	    return;
	  }
	 
	mg_serve_http(nc, hm, s_http_server_opts);
	break;
      }
    case MG_EV_CLOSE:
      {
	//	 nc->flags |=  MG_F_USER_1;
	//has_request=0;
	break;
      }
    default:
      break;
    }


}

static int  last_runstate;
static int  last_runnumber;
static int  last_eventnumber;
static double  last_runvolume;
static int  last_runduration;
static int  last_openflag;
static int  last_current_filename; 



int trigger_updates(struct mg_connection *nc)
{
  if  (  error_flag)
    {
      update(nc, "Status", get_statusstring().c_str());
      error_flag = 0;
    }
  
  if  ( last_runstate   != daq_running() )
    {
      last_runstate  = daq_running();
      update(nc, "LINE", __LINE__);
      update(nc, "RunFlag", daq_running());
      update(nc, "Status", get_statusstring().c_str());
      update(nc, "RunNr", get_runnumber());
      update(nc, "Logging", get_loggingstring().c_str());
    }
  
  if  ( last_openflag != get_openflag() )
    {
      last_openflag = get_openflag();
      update(nc, "Logging", get_loggingstring().c_str());
      update(nc, "OpenFlag", get_openflag());
    }
  
  return 0;
}

void * mg_server (void *arg)
{

  end_web_thread = 0;
  struct mg_mgr mgr;
  struct mg_connection *nc;
  int key = 0;
  int oldkey = 0;
  char str[512];
  int i;

  int port = (int) *(int *)arg;
  stringstream portstring;
  portstring << port << ends;
  
  pthread_mutex_init( &M_ws_send, 0);

  
  last_runstate    = daq_running();
  last_runnumber   = get_runnumber();
  last_eventnumber = get_eventnumber();
  last_runvolume   = get_runvolume();
  last_runduration = get_runduration();
  last_openflag    = get_openflag();
  //  last_current_filename; 


  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, portstring.str().c_str(), ev_handler);
  if (nc == NULL) 
    {
      cerr <<   __FILE__ << " " << __LINE__ <<  " Error starting server on port " << port << endl;
      return 0;
    }
  //   cout  << __FILE__ << " " << __LINE__ <<  " web server started on port " << port << endl;
   
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = HTMLDIR;
  s_http_server_opts.index_files = "control.html";
  s_http_server_opts.enable_directory_listing = "no";
   

  time_t last_time = time(0);
   
  while(!end_web_thread) 
    {
      mg_mgr_poll(&mgr, 1000);
      //       printf ("time %d\n", (int) time(0) );
      if (initial_update_flag)
	{
	  initial_update_flag = 0;
	  initial_ws_update (nc);
	}
      else
	{
	  trigger_updates(nc);
	  if ( time(0) - last_time > 1)
	    {
	      send_ws_updates(nc);
	      last_time = time(0);
	    }
	}
    }
  //   cout  << __FILE__ << " " << __LINE__ <<  " web server is ending"  << endl;
       
  return 0;
}
