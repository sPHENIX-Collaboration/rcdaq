AUTOMAKE_OPTIONS = foreign

allheaders = \
  EvtConstants.h \
  EvtStructures.h \
  oBuffer.h \
  oEvent.h 


allsources = \
  daqEvent.cc \
  daq_device_random.cc \
  rcdaq.cc \
  daqBuffer.cc \
  daq_device.cc 


AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib



INCLUDES = -I$(includedir) -I$(ONLINE_MAIN)/include 

noinst_HEADERS = \
  EvtConstants.h \
  SubevtConstants.h \
  daqBuffer.h \
  daq_device.h \
  EvtStructures.h \
  SubevtStructures.h \
  daqEvent.h \
  daq_device_random.h \
  rcdaq_rpc.h \
  all.h

include_HEADERS = 

RPCGEN = rpcgen


BUILT_SOURCES = \
        rcdaq_rpc_clnt.cc \
        rcdaq_rpc_svc.cc \
        rcdaq_rpc_xdr.cc \
        rcdaq_rpc.h


lib_LTLIBRARIES =  librcdaq.la

librcdaq_la_SOURCES = rcdaq.cc   daqEvent.cc \
  daq_device_random.cc \
  daqBuffer.cc \
  daq_device.cc 


bin_SCRIPTS =  rcdaq_control.pl

bin_PROGRAMS =  rcdaq_client rcdaq_server elogtest

elogtest_SOURCES = elogtest.cc eloghandler.lo


rcdaq_client_SOURCES = rcdaq_client.cc rcdaq_rpc.h
rcdaq_client_LDADD = rcdaq_rpc_clnt.lo rcdaq_rpc_xdr.lo


rcdaq_server_SOURCES = rcdaq_server.cc 
rcdaq_server_LDADD = librcdaq.la rcdaq_rpc_svc.lo rcdaq_rpc_xdr.lo eloghandler.lo -lpthread



#rcdaq_sa_SOURCES = rcdaq_frontend.cc 
#rcdaq_sa_LDADD =  librcdaq.la -lpthread




rcdaq_rpc_clnt.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x .
	$(RPCGEN) -l rcdaq_rpc.x  > $@
	rm rcdaq_rpc.x


rcdaq_rpc_svc.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x .
	$(RPCGEN) -m rcdaq_rpc.x  > $@
	rm rcdaq_rpc.x


rcdaq_rpc_xdr.cc : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x .
	$(RPCGEN)  -c  rcdaq_rpc.x  > $@
	rm rcdaq_rpc.x


rcdaq_rpc.h : $(srcdir)/rcdaq_rpc.x
	cp  $(srcdir)/rcdaq_rpc.x .
	$(RPCGEN)  -h  rcdaq_rpc.x > $@
	rm rcdaq_rpc.x


clean-local:
	rm -f $(BUILT_SOURCES)
