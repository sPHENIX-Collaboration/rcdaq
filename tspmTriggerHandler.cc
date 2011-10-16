
#include "tspmTriggerHandler.h"


int tspmTriggerHandler::wait_for_trigger( const int moreinfo)
{

  int retval = select(_highest_fd+1, _fdset, 0, 0, 0);
  if ( retval >0) 
    {
      return 0;
    }
  return 1;
}
