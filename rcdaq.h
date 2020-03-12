#ifndef __RCDAQ_H__
#define __RCDAQ_H__

#include <iostream>
#include <pthread.h>

class daq_device;

void sig_handler(int i);


void set_eventsizes();

void *writebuffers ( void * arg);
void *EventLoop( void *arg);

int daq_end();
int Command( const int command);


int switch_buffer();
int device_init();
int device_endrun();
int readout(const int etype);
int rearm(const int etype);
int rcdaq_init(pthread_mutex_t &M );
int add_readoutdevice( daq_device *d);

int daq_begin(const int irun,std::ostream& os = std::cout );
int daq_end(std::ostream& os = std::cout);
int daq_fake_trigger (const int n, const int waitinterval);

int daq_write_runnumberfile(const int run);

int daq_set_runnumberfile(const char *file);

int daq_set_filerule(const char *rule);

int daq_setruntype(const char *type, std::ostream& os = std::cout);
int daq_getruntype(const int flag, std::ostream& os = std::cout);
int daq_define_runtype(const char *type, const char * rule);
int daq_list_runtypes(const int flag, std::ostream& os = std::cout );
std::string& daq_get_filerule();


int daq_open(std::ostream& os = std::cout);
int daq_shutdown(const unsigned long servernumber, const unsigned long versionnumber,
		 std::ostream& os = std::cout);

int is_open();

int daq_set_name(const char *name);
int daq_get_name(std::ostream& os = std::cout);
std::string daq_get_myname();

std::string& get_current_filename();
int daq_close (std::ostream& os = std::cout);

int daq_list_readlist(std::ostream& os = std::cout );
int daq_clear_readlist(std::ostream& os = std::cout);
std::string& get_current_filename();
std::string& daq_get_filerule();

int daq_status(const int flag, std::ostream& os = std::cout );
int daq_running();
int daq_status_plugin(const int flag =0, std::ostream& os = std::cout );

int daq_setmaxevents (const int n, std::ostream& os);
int daq_setmaxvolume (const int n_mb, std::ostream& os);

int daq_setmaxbuffersize (const int n_mb, std::ostream& os);

int daq_setadaptivebuffering (const int usecs, std::ostream& os);

int daq_set_eloghandler( const char *host, const int port, const char *logname);

int daq_load_plugin( const char *sharedlib, std::ostream& os);


// functions for the webserver

int daq_webcontrol(const int port,std::ostream& os = std::cout );

int daq_getlastfilename(std::ostream& os = std::cout );


int get_runnumber();
int get_oldrunnumber();
int get_eventnumber();
double get_runvolume();
int get_runduration();
int get_openflag();


#endif
