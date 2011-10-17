#! /bin/sh

# remember this is an executable script. We
# have the full power of a shell script at our
# fingertips.

rcdaq_client daq_clear_readlist

# we add this very file to the begin-run event
rcdaq_client create_device device_file 9 900 $0

# make the first randown device trigger enabled
rcdaq_client create_device device_random 1 1001 64 0 4096 1
rcdaq_client create_device device_random 1 1002 32 0 2048

# see if we have an elog command (ok, this is a weak test but 
# at least it tells us that elog is installed.)
# we still need to supply the elog server whereabouts

rcdaq_client daq_list_readlist

rcdaq_client daq_set_maxbuffersize 16
rcdaq_client daq_status -l


ELOG=`which elog`
[ -n "$ELOG" ]  && rcdaq_client elog localhost 666 RCDAQLog





