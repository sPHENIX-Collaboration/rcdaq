#! /bin/sh

[ -f $HOME/snapshot.jpg ] || exit


if elog -h localhost -p 8080 -l RCDAQLog -w 123 | grep -q "Run.*started" 
then

    # we get the last entry id
    LASTID=$(elog -h localhost -p 8080 -l RCDAQLog -w last | head -n 1 | awk '{print $NF}')
    
    elog -h localhost -p 8080 -l RCDAQLog -e $LASTID -f $HOME/snapshot.jpg > /dev/null 2>&1

fi

