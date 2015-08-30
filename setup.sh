#! /bin/sh


# the following line identifies this script as one to be 
# listed in the configuration GUI

# --RCDAQGUI

# lines starting with the "--SC" tag are short comments 
# (can be only one, later ones override earlier ones)

# --SC Top Level setup script with a random device


# lines starting with the "--SC" tag are long comments 

# --LC This setup script is part of the RCDAQ distribution. 
# --LC It uses the random device (i.e. no particular hardware) 
# --LC and can run on any machine to give RCDAQ a spin.
# --LC It can serve as a template to develop your own setups.

# we need the $0 as absolute path b/c we pass it on to a "file" device further down
D=`dirname "$0"`
B=`basename "$0"`
MYSELF="`cd \"$D\" 2>/dev/null && pwd || echo \"$D\"`/$B"


# remember this is an executable script. We
# have the full power of a shell script at our
# fingertips.

rcdaq_client daq_clear_readlist

# we add this very file to the begin-run event
rcdaq_client create_device device_file 9 900 "$MYSELF"

# make the first randown device trigger enabled
rcdaq_client create_device device_random 1 1001 64 0 4096 1
rcdaq_client create_device device_random 1 1002 32 0 2048
# we add some articicial deadtime to slow down a bit
rcdaq_client create_device device_deadtime 1 0 50000

# see if we have an elog command (ok, this is a weak test but 
# at least it tells us that elog is installed.)
# we still need to supply the elog server whereabouts

rcdaq_client daq_list_readlist

rcdaq_client daq_set_maxbuffersize 16
rcdaq_client daq_status -l


ELOG=$(which elog 2>/dev/null)
[ -n "$ELOG" ]  && rcdaq_client elog localhost 666 RCDAQLog





