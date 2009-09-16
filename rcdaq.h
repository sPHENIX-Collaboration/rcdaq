#ifndef __RCDAQ_H__
#define __RCDAQ_H__

#include <iostream>

class daq_device;

void sig_handler(int i);

int sem_set (const int semnumber, const int value);
int sem_inc (const int semnumber);
int sem_dec (const int semnumber);
int sem_lock (const int semnumber);
int sem_wait (const int semnumber);

void set_eventsizes();

void *writebuffers ( void * arg);
void *EventLoop( void *arg);

int daq_end();
int Command( const int command);


int switch_buffer();
int device_init();
int readout(const int etype);
int rearm(const int etype);
int rcdaq_init( );
int add_readoutdevice( daq_device *d);

int daq_begin(const int irun,std::ostream& os = std::cout );
int daq_end(std::ostream& os = std::cout);
int daq_fake_trigger (const int n, const int waitinterval);

int daq_set_filerule(const char *rule);
std::string& daq_get_filerule();
int daq_open(std::ostream& os = std::cout);
int is_open();
std::string& get_current_filename();
int daq_close (std::ostream& os = std::cout);

int daq_list_readlist(std::ostream& os = std::cout );
int daq_clear_readlist(std::ostream& os = std::cout);
std::string& get_current_filename();
std::string& daq_get_filerule();

int daq_status(const int flag, std::ostream& os = std::cout );

int daq_set_eloghandler( const char *host, const int port, const char *logname);

#endif
