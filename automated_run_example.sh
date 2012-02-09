#! /bin/sh

# this is an example which pretends to step through 10 
# high voltage values and is taking data for each setting.
# this script generates a run-specific description file with
# text and numbers which we add to the begin-run event twice,
# once as a file device in packet 910, and once as a "filenumbers" 
# device, which is extracting the numbers in the file. The former is
# easier for humans to read, the latter is easier for the analysis code
# to get hold of. 
# This demonstrates - 
# - a fully automated run sequence
# - adding external information to the begin-run event 

D=`pwd`

if ! rcdaq_client daq_list_readlist | grep -q 'Event Type: 9 Subevent id: 911'
then

rcdaq_client create_device device_file 9 910 ${D}/description.txt
rcdaq_client create_device device_filenumbers  9 911 ${D}/description.txt

fi

rcdaq_client daq_list_readlist

sleep 5;

INITIAL_RUN=$1

[ -n "$INITIAL_RUN" ] || INITIAL_RUN=1000

RUN=$INITIAL_RUN

for highvoltage  in $( seq 1500 10 1600 ) ; do

# here you would do something to actually set the HV

 cat >> ${D}/description.txt <<EOF
This is Run $RUN with the High Voltage set to 
$highvoltage
This uses sample number
5
of the calorimeter.
EOF

 echo "starting run $RUN with High Voltage set to $highvoltage"

 rcdaq_client daq_begin $RUN
 
 # just a placeholder for some useful run end
 sleep 5;
 rm -f ${D}/description.txt
 rcdaq_client daq_end

 # now go to the next run number
 RUN=`expr $RUN + 1`

done
