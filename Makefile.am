AUTOMAKE_OPTIONS = foreign

allheaders = \
  EvtConstants.h \
  EvtStructures.h \
  TriggerHandler.h \
  rcdaq_plugin.h \
  pulserTriggerHandler.h \
  oBuffer.h \
  oEvent.h 


allsources = \
  daqEvent.cc \
  daq_device_random.cc \
  daq_device_file.cc \
  daq_device_filenumbers.cc \
  rcdaq.cc \
  daqBuffer.cc \
  daq_device.cc 


AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib



INCLUDES = -I$(includedir) -I$(ONLINE_MAIN)/include 

noinst_HEADERS = \
  EvtConstants.h \
  daqBuffer.h \
  EvtStructures.h \
  daqEvent.h \
  daq_device_random.h \
  daq_device_file.h \
  daq_device_filenumbers.h \
  all.h

include_HEADERS =  \
  SubevtStructures.h \
  SubevtConstants.h \
  daq_device.h \
  rcdaq_rpc.h \
  TriggerHandler.h \
  rcdaq_plugin.h


RPCGEN = rpcgen


BUILT_SOURCES = \
        rcdaq_rpc_clnt.cc \
        rcdaq_rpc_svc.cc \
        rcdaq_rpc_xdr.cc \
        rcdaq_rpc.h


lib_LTLIBRARIES =  librcdaq.la  librcdaqplugin_example.la

librcdaq_la_SOURCES = rcdaq.cc   daqEvent.cc \
  daq_device_random.cc \
  daq_device_file.cc \
  daq_device_filenumbers.cc \
  daqBuffer.cc \
  daq_device.cc 

librcdaqplugin_example_la_SOURCES = daq_device_pluginexample.cc example_plugin.cc

bin_SCRIPTS =  rcdaq_control.pl aliases.csh automated_run_example.sh

bin_PROGRAMS =  rcdaq_client rcdaq_server elogtest

elogtest_SOURCES = elogtest.cc
elogtest_LDADD = eloghandler.lo 


rcdaq_client_SOURCES = rcdaq_client.cc rcdaq_rpc.h
rcdaq_client_LDADD = rcdaq_rpc_clnt.lo rcdaq_rpc_xdr.lo


rcdaq_server_SOURCES = rcdaq_server.cc 
rcdaq_server_LDADD = librcdaq.la rcdaq_rpc_svc.lo rcdaq_rpc_xdr.lo eloghandler.lo -lpthread -ldl -ltspm

rcdaq_server_LDFLAGS =  -Wl,-export-dynamic

#rcdaq_sa_SOURCES = rcdaq_frontend.cc 
#rcdaq_sa_LDADD =  librcdaq.la -lpthread




rcdaq_rpc_clnt.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x ./rcdaq_rpc.xx
	$(RPCGEN) -l rcdaq_rpc.xx  > $@
	rm rcdaq_rpc.xx


rcdaq_rpc_svc.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x ./rcdaq_rpc.xx
	$(RPCGEN) -m rcdaq_rpc.xx  > $@
	rm rcdaq_rpc.xx


rcdaq_rpc_xdr.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x ./rcdaq_rpc.xx
	$(RPCGEN)  -c  rcdaq_rpc.xx  > $@
	rm rcdaq_rpc.xx


rcdaq_rpc.h : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x ./rcdaq_rpc.xx
	$(RPCGEN)  -h  rcdaq_rpc.xx > $@
	rm rcdaq_rpc.xx


clean-local:
	rm -f $(BUILT_SOURCES)
