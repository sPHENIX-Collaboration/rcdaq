#! /bin/sh

# we wait for the end of the run or at most n seconds, if specified 

if ! rcdaq_client daq_status -s > /dev/null 2>&1 ; then
    exit 1;
fi


MAX_TIME=$1

[ -z "$MAX_TIME" ] && MAX_TIME=0

# we record the start time 
START=$(date +%s)

while true; do 

    if [ $MAX_TIME -gt 0 ] ; then
	NOW=$(date +%s)
	DELTA=$(expr $NOW - $START)
	#echo "$START $NOW $DELTA"
	[ $DELTA -ge $MAX_TIME ] && exit 1 
    fi

    S=$(rcdaq_client daq_status -s 2>/dev/null | awk '{print $1}')
#    echo $S

    [ -z "$S" -o "$S" = "-1" ] && exit 0
    sleep 2

done

 
 