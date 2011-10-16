#ifndef __PULSERTRIGGERHANDLER_H__
#define __PULSERTRIGGERHANDLER_H__

#include <unistd.h>

class pulserTriggerHandler : public TriggerHandler {

public:

  pulserTriggerHandler(const int sleeptime = 10000)
    {
      _sleeptime = sleeptime;
      _count = 0;
    }

  ~pulserTriggerHandler() {};

  //  virtual void identify(std::ostream& os = std::cout) const = 0;

  // this is the virtual worker routine
  int wait_for_trigger( const int moreinfo=0)
  {
    int sleeptime = _sleeptime;
    if ( moreinfo) sleeptime = moreinfo;
    std::cout << "trigger " << _count++ << std::endl; 
    usleep ( sleeptime);
    return 0;
  }
 protected:
  
  int _sleeptime;
  int _count;

};

#endif
