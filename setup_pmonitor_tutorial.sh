#! /bin/sh

#
# mlp -- this is the rcdaq setup you can use to run the
#        tutorials in the pmonitor manual. 
#

# we need the $0 as absolute path 
D=`dirname "$0"`
B=`basename "$0"`
MYSELF="`cd \"$D\" 2>/dev/null && pwd || echo \"$D\"`/$B"

# we figure out if a server is already running
if ! rcdaq_client daq_status > /dev/null 2>&1 ; then

    echo "No rcdaq_server running, starting... log goes to $HOME/rcdaq.log"
    rcdaq_server > $HOME/rcdaq.log 2>&1 &
    sleep 2

fi

rcdaq_client load librcdaqplugin_gauss.so

rcdaq_client daq_clear_readlist

# we add this very file to the begin-run event
rcdaq_client create_device device_file 9 900 "$MYSELF"

# make the gauss device, and trigger-enable it
rcdaq_client create_device device_gauss -- 1 1003 1

# we add some artificial deadtime to slow down a bit
rcdaq_client create_device device_deadtime 1 0 5000

