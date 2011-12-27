
#include "tspmTriggerHandler.h"
#include <iostream>

using namespace std;


int tspmTriggerHandler::wait_for_trigger( const int moreinfo)
{

  //  cout << "waiting for data" << endl;
  int retval = select(_highest_fd+1, _fdset, 0, 0, 0);
  //cout << "got data" << endl;
  if ( retval >0) 
    {
      return 0;
    }
  return 1;
}
