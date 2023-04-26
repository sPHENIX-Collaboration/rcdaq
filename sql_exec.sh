#! /bin/bash

# in brief (this is heavily geared to the sPHENIX DB environment):
#   - make a named fifo, such as "mkfifo sqlinput"
#   - tell RCDAQ to write to that fifo - "rcdaq_client daq_open_sqlstream sqlinput"
# then this script will grab the input and fill the DB

# if another fifo name is wanted, give it as parameter

FIFO="$1"
[ -z "$FIFO" ] && FIFO=sqlinput

# define the actual database tables in use here
export FILETABLE=filelist
export RUNTABLE=run

# define the fifo
FIFO

while true ; do
    while read -r line ; do
	xline=$(echo $line | envsubst)
	echo $xline
	psql -U phnxrc play -c "$xline"
    done < "$FIFO"
done
