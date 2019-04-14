
#include "mongoose.h"
#include "rcdaq.h"

#include <iostream>
#include <sstream>
#include <sys/stat.h>

using namespace std;
	     
static struct mg_serve_http_opts s_http_server_opts;

int end_web_thread = 0;

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
  if ( daq_running())
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


void initial_update (struct mg_connection *nc)
{
  char str[512];
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
  
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
	    "Content-Type: application/json\r\n\r\n%s",
	    len, str);
  
}

  
void send_updates (struct mg_connection *nc)
{
  char str[512];
  int len;
  static int evt=10;
  len = sprintf(str, "{ \"Events\":%d , \"Volume\":\"%f\" , \"Status\":\"%s\" , \"Name\":\"%s\" }"
		, get_eventnumber()
		, get_runvolume()
		, get_statusstring().c_str()
		, daq_get_myname().c_str()
		);


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

  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
	    "Content-Type: application/json\r\n\r\n%s",
	    len, str);
  
}

void send_nothing (struct mg_connection *nc)
{
  nc->flags |= MG_F_SEND_AND_CLOSE;
  mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: 0\r\n"
	    "Content-Type: text/html\r\n\r\n");
  
}


static void broadcast(struct mg_connection *nc, char *str, const int len)
{
   struct mg_connection *c;

   for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c))
     {
       if ( c->flags & MG_F_USER_1 )
	 {
	   mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, str, len);
	 }
     }
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

     case MG_EV_HTTP_REQUEST:
       {
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
	 
	 else if ( mg_vcmp ( &hm->uri, "/initial_update") == 0)
	   {
	     initial_update(nc);
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

int trigger_updates(struct mg_connection *nc)
{
  if  ( last_runstate   != daq_running() )
    {
      last_runstate  = daq_running();
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
       if ( time(0) - last_time > 2)
	 {
	   trigger_updates(nc);
	   last_time = time(0);
	 }
     }
   //   cout  << __FILE__ << " " << __LINE__ <<  " web server is ending"  << endl;
       
   return 0;
}
