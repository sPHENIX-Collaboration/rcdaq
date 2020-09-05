AUTOMAKE_OPTIONS = foreign

allheaders = \
  EvtConstants.h \
  EvtStructures.h \
  TriggerHandler.h \
  rcdaq_plugin.h \
  pulserTriggerHandler.h \
  oBuffer.h \
  oEvent.h \
  parseargument.h


allsources = \
  daqEvent.cc \
  daqONCSEvent.cc \
  daq_device_random.cc \
  daq_device_deadtime.cc \
  daq_device_file.cc \
  daq_device_filenumbers.cc \
  daq_device_command.cc \
  daq_device_rtclock.cc \
  rcdaq.cc \
  daqBuffer.cc \
  rcdaq_mg_server.cc \
  mongoose.cc \
  daq_device.cc 


AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib

INCLUDES = $(AM_CPPFLAGS) -I$(includedir) -I$(ONLINE_MAIN)/include $(RPC_INCLUDE)

noinst_HEADERS = \
  EvtConstants.h \
  daqBuffer.h \
  EvtStructures.h \
  daqEvent.h \
  daqONCSEvent.h \
  daq_device_random.h \
  daq_device_deadtime.h \
  daq_device_file.h \
  daq_device_filenumbers.h \
  daq_device_command.h \
  daq_device_rtclock.h \
  mongoose.h \
  all.h

include_HEADERS =  \
  SubevtStructures.h \
  SubevtConstants.h \
  daq_device.h \
  rcdaq_rpc.h \
  TriggerHandler.h \
  rcdaq_plugin.h \
  parseargument.h

htmldir = $(datadir)/html
html_DATA = control.html  favicon.ico
html_INDEX = control.html

RPCGEN = rpcgen


BUILT_SOURCES = \
        rcdaq_rpc_clnt.cc \
        rcdaq_rpc_svc.cc \
        rcdaq_rpc_xdr.cc \
        rcdaq_rpc.h


lib_LTLIBRARIES =  librcdaq.la librcdaqutils.la  librcdaqplugin_example.la librcdaqplugin_gauss.la

librcdaq_la_SOURCES = rcdaq.cc \
  daqEvent.cc \
  daqONCSEvent.cc \
  daqPRDFEvent.cc \
  daq_device_random.cc \
  daq_device_deadtime.cc \
  daq_device_file.cc \
  daq_device_filenumbers.cc \
  daq_device_command.cc \
  daq_device_rtclock.cc \
  daqBuffer.cc \
  daq_device.cc \
  rcdaq_mg_server.cc \
  mongoose.cc \
  parseargument.cc


librcdaq_la_CPPFLAGS = -DHTMLFILE=\"$(html_INDEX)\" -DHTMLDIR=\"$(htmldir)\"

librcdaqutils_la_SOURCES =  parseargument.cc

librcdaqplugin_example_la_SOURCES = daq_device_pluginexample.cc example_plugin.cc

librcdaqplugin_gauss_la_SOURCES = daq_device_gauss.cc gauss_plugin.cc simpleRandom.cc

bin_SCRIPTS =  rcdaq_control.pl rcdaq_status.pl rcdaq_runtypechooser.pl rcdaq_configmenu.pl aliases.csh aliases.sh automated_run_example.sh wait_for_run_end.sh

bin_PROGRAMS =  rcdaq_client rcdaq_server elogtest

elogtest_SOURCES = elogtest.cc
elogtest_LDADD = eloghandler.lo 


rcdaq_client_SOURCES = rcdaq_client.cc  rcdaq_rpc.h
rcdaq_client_LDADD = rcdaq_rpc_clnt.lo rcdaq_rpc_xdr.lo parseargument.lo $(RPC_LDD)


rcdaq_server_SOURCES = rcdaq_server.cc 
rcdaq_server_LDADD = librcdaq.la rcdaq_rpc_svc.lo rcdaq_rpc_xdr.lo eloghandler.lo $(RPC_LDD) -lpthread -ldl 

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
