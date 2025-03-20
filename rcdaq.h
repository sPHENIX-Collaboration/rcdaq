#ifndef __RCDAQ_H__
#define __RCDAQ_H__

#include <iostream>
#include <pthread.h>

class daq_device;

void sig_handler(int i);


void set_eventsizes();

void *writebuffers ( void * arg);
void *EventLoop( void *arg);

//int daq_end();
int Command( const int command);

char * obtain_pidfilename();

int switch_buffer();
int device_init();
int device_endrun();
int readout(const int etype);
int rearm(const int etype);
int rcdaq_init(const int, pthread_mutex_t &M );
int add_readoutdevice( daq_device *d);

int register_fd_use (const int fd, const int flag);


int daq_set_nr_threads (const int n, std::ostream& os);
int daq_set_number_of_write_threads(const int n);
  
int daq_begin(const int irun,std::ostream& os = std::cout );
int daq_begin_immediate(const int irun, std::ostream& os);
void * daq_begin_thread( void *arg);


int daq_end_immediate(std::ostream& os = std::cout);
int daq_end_interactive(std::ostream& os);
int daq_end(std::ostream& os = std::cout);

int daq_fake_trigger (const int n, const int waitinterval);

int daq_write_runnumberfile(const int run);

int daq_set_runnumberfile(const char *file, const int flag);
int daq_set_runnumberApp(const char *file, const int flag);

int daq_set_filerule(const char *rule, std::ostream& os = std::cout);

int daq_setruntype(const char *type, std::ostream& os = std::cout);
int daq_getruntype(const int flag, std::ostream& os = std::cout);
int daq_define_runtype(const char *type, const char * rule);
int daq_list_runtypes(const int flag, std::ostream& os = std::cout );
std::string& daq_get_filerule();


int daq_open(std::ostream& os = std::cout);
int daq_shutdown(const unsigned long servernumber, const unsigned long versionnumber, const int pid_fd,
		 std::ostream& os = std::cout);
std::string& get_current_filename();
int daq_close (std::ostream& os = std::cout);
int is_open();

int daq_set_server (const char *hostname, const int port, std::ostream& os = std::cout);
int is_server_open();
int daq_server_close (std::ostream& os = std::cout);
int get_serverflag();

int daq_set_compression(const int flag, std::ostream& os = std::cout);
int daq_set_md5enable (const int flag, std::ostream& os = std::cout);
int daq_set_md5allowturnoff (const int flag, std::ostream& os = std::cout);


int daq_set_name(const char *name);
int daq_get_name(std::ostream& os = std::cout);
std::string daq_get_myname();

int daq_set_mqtt_host(const char * host, const int port, std::ostream& os);
int daq_get_mqtt_host(std::ostream& os);

//int daq_open_sqlstream(const char *name);
//int daq_close_sqlstream();
//int get_sqlfd();

int daq_generate_json (const int flag);

double daq_get_mb_per_second();
double daq_get_events_per_second();

int daq_list_readlist(std::ostream& os = std::cout );
int daq_clear_readlist(std::ostream& os = std::cout);
std::string& get_current_filename();
std::string& daq_get_filerule();

int daq_status(const int flag, std::ostream& os = std::cout );
int daq_running();
int daq_status_plugin(const int flag =0, std::ostream& os = std::cout );

int daq_setmaxevents (const int n, std::ostream& os);
int daq_setmaxvolume (const int n_mb, std::ostream& os);
int daq_setrolloverlimit (const int n_gb, std::ostream& os);

int daq_setmaxbuffersize (const int n_mb, std::ostream& os);
int daq_define_buffersize (const int n_mb, std::ostream& os);

int daq_setadaptivebuffering (const int usecs, std::ostream& os);

int daq_set_eloghandler( const char *host, const int port, const char *logname);

int daq_load_plugin( const char *sharedlib, std::ostream& os);

int daq_wait_for_begin_done();
int daq_wait_for_actual_end();

// functions for the webserver

int daq_webcontrol(const int port,std::ostream& os = std::cout );

int daq_getlastfilename(std::ostream& os = std::cout );
int daq_getlastevent_number(std::ostream& os = std::cout );


int get_runnumber();
int get_oldrunnumber();
int get_eventnumber();
double get_runvolume();
int get_runduration();
int get_openflag();

int daq_setEventFormat(const int f, std::ostream& os = std::cout );
int daq_getEventFormat();
int daq_setRunControlMode ( const int flag, std::ostream& os = std::cout );
int daq_getRunControlMode (std::ostream& os = std::cout);

int getRunNumberFromApp();

int daq_set_uservalue ( const int index, const int value, std::ostream& os = std::cout );
int daq_get_uservalue ( const int index,  std::ostream& os = std::cout );
int get_uservalue ( const int index);



#define MG_REQUEST_NAME 1
#define MG_REQUEST_SPEED 2
// more defs to come in the future



#endif
