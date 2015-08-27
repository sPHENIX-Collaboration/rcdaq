#! /bin/sh

# this is an example script that is meant to be added as a device_command.

# We show how we are adding a previously generated image to an 
# existing elog entry as an attachment. It shows the use of the environmental variables
# which cdaq defines for the use in such scripts. 

# if elog is not defined, we do nothing
[ -z "$DAQ_ELOGHOST ] && exit

[ -f $HOME/snapshot.jpg ] || exit


if elog -h $DAQ_ELOGHOST  -p $DAQ_ELOGPORT -l $DAQ_ELOGLOGBOOK -w last | grep -q "Run.*started" 
then

    # we get the last entry id
    LASTID=$(elog -h $DAQ_ELOGHOST  -p $DAQ_ELOGPORT -l $DAQ_ELOGLOGBOOK -w last | head -n 1 | awk '{print $NF}')
    
    elog -h $DAQ_ELOGHOST  -p $DAQ_ELOGPORT -l $DAQ_ELOGLOGBOOK -e $LASTID -f $HOME/snapshot.jpg > /dev/null 2>&1

fi

